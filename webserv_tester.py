#!/usr/bin/env python3
"""
Testeur complet pour Webserv - Projet 42
Teste toutes les fonctionnalités HTTP selon le sujet
"""

import socket
import time
import threading
import subprocess
import signal
import os
import sys
import json
import random
import string
from datetime import datetime
from urllib.parse import urlparse
import tempfile

class Colors:
    GREEN = '\033[92m'
    RED = '\033[91m'
    YELLOW = '\033[93m'
    BLUE = '\033[94m'
    MAGENTA = '\033[95m'
    CYAN = '\033[96m'
    WHITE = '\033[97m'
    BOLD = '\033[1m'
    END = '\033[0m'

class WebservTester:
    def __init__(self, config_file="config.conf", host="localhost", port=8081):
        self.config_file = config_file
        self.host = host
        self.port = port
        self.webserv_process = None
        self.results = {
            'total_tests': 0,
            'passed': 0,
            'failed': 0,
            'tests': []
        }
        
    def log(self, message, color=Colors.WHITE):
        timestamp = datetime.now().strftime("%H:%M:%S")
        print(f"{color}[{timestamp}] {message}{Colors.END}")
        
    def log_success(self, message):
        self.log(f"✅ {message}", Colors.GREEN)
        
    def log_error(self, message):
        self.log(f"❌ {message}", Colors.RED)
        
    def log_warning(self, message):
        self.log(f"⚠️  {message}", Colors.YELLOW)
        
    def log_info(self, message):
        self.log(f"ℹ️  {message}", Colors.BLUE)
        
    def add_test_result(self, test_name, passed, details=""):
        self.results['total_tests'] += 1
        if passed:
            self.results['passed'] += 1
            self.log_success(f"{test_name}: PASSED")
        else:
            self.results['failed'] += 1
            self.log_error(f"{test_name}: FAILED - {details}")
            
        self.results['tests'].append({
            'name': test_name,
            'passed': passed,
            'details': details,
            'timestamp': datetime.now().isoformat()
        })
        
    def start_webserv(self):
        """Démarre le serveur webserv"""
        self.log_info(f"Démarrage de webserv avec {self.config_file}")
        try:
            self.webserv_process = subprocess.Popen(
                ["./webserv", self.config_file],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                preexec_fn=os.setsid
            )
            time.sleep(2)  # Attendre que le serveur démarre
            
            # Vérifier si le serveur est bien démarré
            if self.webserv_process.poll() is not None:
                stdout, stderr = self.webserv_process.communicate()
                raise Exception(f"Webserv a crashé: {stderr.decode()}")
                
            self.log_success("Webserv démarré avec succès")
            return True
        except Exception as e:
            self.log_error(f"Impossible de démarrer webserv: {e}")
            return False
            
    def stop_webserv(self):
        """Arrête le serveur webserv"""
        if self.webserv_process:
            try:
                os.killpg(os.getpgid(self.webserv_process.pid), signal.SIGTERM)
                self.webserv_process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                os.killpg(os.getpgid(self.webserv_process.pid), signal.SIGKILL)
                self.webserv_process.wait()
            except:
                pass
            self.webserv_process = None
            self.log_info("Webserv arrêté")
            
    def send_http_request(self, method, path, headers=None, body="", timeout=10):
        """Envoie une requête HTTP brute"""
        if headers is None:
            headers = {}
            
        # Headers par défaut
        default_headers = {
            "Host": f"{self.host}:{self.port}",
            "User-Agent": "WebservTester/1.0",
            "Connection": "close"
        }
        default_headers.update(headers)
        
        # Construction de la requête
        request = f"{method} {path} HTTP/1.1\r\n"
        for name, value in default_headers.items():
            request += f"{name}: {value}\r\n"
        request += "\r\n"
        
        if body:
            request += body
            
        # Envoi de la requête
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.settimeout(timeout)
            sock.connect((self.host, self.port))
            sock.sendall(request.encode())
            
            response = b""
            while True:
                try:
                    data = sock.recv(4096)
                    if not data:
                        break
                    response += data
                except socket.timeout:
                    break
                    
            sock.close()
            return response.decode('utf-8', errors='ignore')
            
        except Exception as e:
            return f"ERROR: {str(e)}"
            
    def parse_http_response(self, response):
        """Parse une réponse HTTP"""
        if response.startswith("ERROR:"):
            return None, None, response
            
        lines = response.split('\r\n')
        if not lines:
            return None, None, "Empty response"
            
        # Status line
        status_line = lines[0]
        parts = status_line.split(' ', 2)
        if len(parts) < 2:
            return None, None, "Invalid status line"
            
        status_code = int(parts[1])
        
        # Headers
        headers = {}
        body_start = 0
        for i, line in enumerate(lines[1:], 1):
            if line == "":
                body_start = i + 1
                break
            if ':' in line:
                name, value = line.split(':', 1)
                headers[name.strip().lower()] = value.strip()
                
        # Body
        body = '\r\n'.join(lines[body_start:])
        
        return status_code, headers, body
        
    def test_basic_get(self):
        """Test GET basique"""
        response = self.send_http_request("GET", "/")
        status_code, headers, body = self.parse_http_response(response)
        
        if status_code == 200:
            self.add_test_result("Basic GET", True)
        else:
            self.add_test_result("Basic GET", False, f"Status: {status_code}")
            
    def test_static_file(self):
        """Test de fichier statique"""
        response = self.send_http_request("GET", "/index.html")
        status_code, headers, body = self.parse_http_response(response)
        
        passed = status_code == 200 and len(body) > 0
        self.add_test_result("Static File", passed, f"Status: {status_code}")
        
    def test_404_error(self):
        """Test page d'erreur 404"""
        response = self.send_http_request("GET", "/nonexistent.html")
        status_code, headers, body = self.parse_http_response(response)
        
        passed = status_code == 404
        self.add_test_result("404 Error", passed, f"Status: {status_code}")
        
    def test_post_method(self):
        """Test méthode POST"""
        body = "test=data&name=value"
        headers = {"Content-Type": "application/x-www-form-urlencoded", "Content-Length": str(len(body))}
        response = self.send_http_request("POST", "/", headers, body)
        status_code, headers, body = self.parse_http_response(response)
        
        passed = status_code in [200, 201, 405]  # 405 si POST non autorisé
        self.add_test_result("POST Method", passed, f"Status: {status_code}")
        
    def test_delete_method(self):
        """Test méthode DELETE"""
        response = self.send_http_request("DELETE", "/test.txt")
        status_code, headers, body = self.parse_http_response(response)
        
        passed = status_code in [200, 204, 404, 405]  # Différents codes possibles
        self.add_test_result("DELETE Method", passed, f"Status: {status_code}")
        
    def test_cgi_execution(self):
        """Test exécution CGI"""
        response = self.send_http_request("GET", "/aaa.cgi.py")
        status_code, headers, body = self.parse_http_response(response)
        
        if status_code == 200 and "content-type" in headers:
            self.add_test_result("CGI Execution", True)
        else:
            self.add_test_result("CGI Execution", False, f"Status: {status_code}, Headers: {headers}")
            
    def test_cgi_post(self):
        """Test CGI avec POST"""
        body = "name=test&value=42"
        headers = {"Content-Type": "application/x-www-form-urlencoded", "Content-Length": str(len(body))}
        response = self.send_http_request("POST", "/aaa.cgi.py", headers, body)
        status_code, headers, body = self.parse_http_response(response)
        
        passed = status_code == 200
        self.add_test_result("CGI POST", passed, f"Status: {status_code}")
        
    def test_autoindex(self):
        """Test autoindex"""
        response = self.send_http_request("GET", "/directory/")
        status_code, headers, body = self.parse_http_response(response)
        
        # Chercher des signes d'autoindex dans la réponse
        has_autoindex = "Index of" in body or "<ul>" in body or "Directory" in body
        passed = status_code == 200 and has_autoindex
        self.add_test_result("Autoindex", passed, f"Status: {status_code}")
        
    def test_large_body(self):
        """Test avec un body volumineux"""
        large_body = "x" * 2000000  # 2MB
        headers = {"Content-Type": "text/plain", "Content-Length": str(len(large_body))}
        response = self.send_http_request("POST", "/", headers, large_body, timeout=30)
        status_code, headers, body = self.parse_http_response(response)
        
        # Peut retourner 413 (trop large) ou être accepté
        passed = status_code in [200, 201, 413]
        self.add_test_result("Large Body", passed, f"Status: {status_code}")
        
    def test_multiple_headers(self):
        """Test avec plusieurs headers"""
        headers = {
            "X-Custom-Header": "test-value",
            "X-Another-Header": "another-value",
            "Accept": "text/html,application/xhtml+xml",
            "Accept-Language": "en-US,en;q=0.9"
        }
        response = self.send_http_request("GET", "/", headers)
        status_code, headers, body = self.parse_http_response(response)
        
        passed = status_code == 200
        self.add_test_result("Multiple Headers", passed, f"Status: {status_code}")
        
    def test_chunked_request(self):
        """Test requête chunked"""
        # Construction d'une requête chunked
        chunk1 = "Hello"
        chunk2 = "World"
        
        chunked_body = f"{len(chunk1):x}\r\n{chunk1}\r\n{len(chunk2):x}\r\n{chunk2}\r\n0\r\n\r\n"
        headers = {"Transfer-Encoding": "chunked"}
        
        response = self.send_http_request("POST", "/", headers, chunked_body)
        status_code, headers, body = self.parse_http_response(response)
        
        passed = status_code in [200, 201, 400]  # Peut ne pas supporter chunked
        self.add_test_result("Chunked Request", passed, f"Status: {status_code}")
        
    def test_concurrent_connections(self, num_connections=10):
        """Test connexions concurrentes"""
        results = []
        threads = []
        
        def make_request():
            response = self.send_http_request("GET", "/")
            status_code, _, _ = self.parse_http_response(response)
            results.append(status_code == 200)
            
        for _ in range(num_connections):
            thread = threading.Thread(target=make_request)
            threads.append(thread)
            thread.start()
            
        for thread in threads:
            thread.join()
            
        success_rate = sum(results) / len(results) if results else 0
        passed = success_rate >= 0.8  # Au moins 80% de succès
        self.add_test_result(f"Concurrent Connections ({num_connections})", passed, 
                           f"Success rate: {success_rate:.2f}")
                           
    def test_timeout_handling(self):
        """Test gestion des timeouts"""
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.connect((self.host, self.port))
            
            # Envoyer une requête incomplète
            incomplete_request = "GET / HTTP/1.1\r\nHost: localhost\r\n"
            sock.send(incomplete_request.encode())
            
            # Attendre et voir si le serveur ferme la connexion
            time.sleep(15)  # Attendre plus longtemps que le timeout
            
            try:
                data = sock.recv(1024)
                if not data:
                    passed = True  # Connexion fermée par timeout
                else:
                    passed = False  # Serveur encore actif
            except:
                passed = True  # Connexion fermée
                
            sock.close()
            self.add_test_result("Timeout Handling", passed)
            
        except Exception as e:
            self.add_test_result("Timeout Handling", False, str(e))
            
    def test_malformed_requests(self):
        """Test requêtes malformées"""
        malformed_requests = [
            "INVALID REQUEST\r\n\r\n",
            "GET\r\n\r\n",
            "GET / HTTP/2.0\r\n\r\n",
            "GET / HTTP/1.1\r\nHost: \r\n\r\n",
            "\r\n\r\n"
        ]
        
        passed_count = 0
        for req in malformed_requests:
            try:
                sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                sock.settimeout(5)
                sock.connect((self.host, self.port))
                sock.send(req.encode())
                
                response = sock.recv(1024).decode()
                # Le serveur doit retourner 400 ou fermer la connexion
                if "400" in response or not response:
                    passed_count += 1
                    
                sock.close()
            except:
                passed_count += 1  # Connexion fermée = bon comportement
                
        passed = passed_count >= len(malformed_requests) * 0.6  # 60% minimum
        self.add_test_result("Malformed Requests", passed, 
                           f"{passed_count}/{len(malformed_requests)} handled correctly")
                           
    def test_file_upload(self):
        """Test upload de fichier"""
        boundary = "----WebservTesterBoundary"
        filename = "test_upload.txt"
        file_content = "This is a test file for upload\nSecond line"
        
        body = f"--{boundary}\r\n"
        body += f'Content-Disposition: form-data; name="file"; filename="{filename}"\r\n'
        body += "Content-Type: text/plain\r\n\r\n"
        body += file_content
        body += f"\r\n--{boundary}--\r\n"
        
        headers = {
            "Content-Type": f"multipart/form-data; boundary={boundary}",
            "Content-Length": str(len(body))
        }
        
        response = self.send_http_request("POST", "/", headers, body)
        status_code, headers, body = self.parse_http_response(response)
        
        passed = status_code in [200, 201]
        self.add_test_result("File Upload", passed, f"Status: {status_code}")
        
    def stress_test(self, duration=30, concurrent_connections=20):
        """Test de stress"""
        self.log_info(f"Démarrage du stress test ({duration}s, {concurrent_connections} connexions)")
        
        start_time = time.time()
        request_count = 0
        error_count = 0
        lock = threading.Lock()
        
        def stress_worker():
            nonlocal request_count, error_count
            while time.time() - start_time < duration:
                try:
                    response = self.send_http_request("GET", "/")
                    status_code, _, _ = self.parse_http_response(response)
                    
                    with lock:
                        request_count += 1
                        if status_code != 200:
                            error_count += 1
                            
                except:
                    with lock:
                        error_count += 1
                        
                time.sleep(0.1)  # Petite pause
                
        threads = []
        for _ in range(concurrent_connections):
            thread = threading.Thread(target=stress_worker)
            threads.append(thread)
            thread.start()
            
        for thread in threads:
            thread.join()
            
        error_rate = error_count / request_count if request_count > 0 else 1
        requests_per_second = request_count / duration
        
        passed = error_rate < 0.1 and request_count > 0  # Moins de 10% d'erreurs
        self.add_test_result("Stress Test", passed, 
                           f"RPS: {requests_per_second:.2f}, Error rate: {error_rate:.2%}")
                           
    def run_all_tests(self):
        """Exécute tous les tests"""
        self.log_info("🚀 Démarrage de la suite de tests complète")
        
        if not self.start_webserv():
            self.log_error("Impossible de démarrer webserv. Tests annulés.")
            return
            
        try:
            # Tests de base
            self.log_info("=== Tests de base ===")
            self.test_basic_get()
            self.test_static_file()
            self.test_404_error()
            
            # Tests des méthodes HTTP
            self.log_info("=== Tests des méthodes HTTP ===")
            self.test_post_method()
            self.test_delete_method()
            
            # Tests CGI
            self.log_info("=== Tests CGI ===")
            self.test_cgi_execution()
            self.test_cgi_post()
            
            # Tests des fonctionnalités
            self.log_info("=== Tests des fonctionnalités ===")
            self.test_autoindex()
            self.test_multiple_headers()
            self.test_file_upload()
            
        finally:
            self.stop_webserv()
            
        self.print_results()
        
    def print_results(self):
        """Affiche les résultats des tests"""
        print("\n" + "="*80)
        print(f"{Colors.BOLD}RÉSULTATS DES TESTS{Colors.END}")
        print("="*80)
        
        total = self.results['total_tests']
        passed = self.results['passed']
        failed = self.results['failed']
        success_rate = (passed / total * 100) if total > 0 else 0
        
        print(f"Total des tests: {total}")
        print(f"{Colors.GREEN}Tests réussis: {passed}{Colors.END}")
        print(f"{Colors.RED}Tests échoués: {failed}{Colors.END}")
        print(f"Taux de réussite: {Colors.GREEN if success_rate >= 80 else Colors.RED}{success_rate:.1f}%{Colors.END}")
        
        if failed > 0:
            print(f"\n{Colors.RED}TESTS ÉCHOUÉS:{Colors.END}")
            for test in self.results['tests']:
                if not test['passed']:
                    print(f"  ❌ {test['name']}: {test['details']}")
                    
        print("\n" + "="*80)
        
        # Sauvegarde des résultats
        with open("test_results.json", "w") as f:
            json.dump(self.results, f, indent=2)
        self.log_info("Résultats sauvegardés dans test_results.json")

def main():
    if len(sys.argv) > 1:
        config_file = sys.argv[1]
    else:
        config_file = "config.conf"
        
    tester = WebservTester(config_file)
    
    print(f"{Colors.BOLD}{Colors.BLUE}")
    print("="*80)
    print("           TESTEUR COMPLET WEBSERV - PROJET 42")
    print("="*80)
    print(f"{Colors.END}")
    
    try:
        tester.run_all_tests()
    except KeyboardInterrupt:
        print(f"\n{Colors.YELLOW}Tests interrompus par l'utilisateur{Colors.END}")
        tester.stop_webserv()
    except Exception as e:
        print(f"{Colors.RED}Erreur durant les tests: {e}{Colors.END}")
        tester.stop_webserv()

if __name__ == "__main__":
    main()
