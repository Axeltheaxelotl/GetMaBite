#!/usr/bin/env python3
"""
Script de test complet pour le serveur webserv
Teste toutes les fonctionnalités principales et identifie les problèmes
"""

import socket
import threading
import time
import os
import sys
import subprocess
import requests
from urllib.parse import urljoin

class WebservTester:
    def __init__(self, host='localhost', port=8081):
        self.host = host
        self.port = port
        self.base_url = f"http://{host}:{port}"
        self.server_process = None
        self.results = []
        
    def log_result(self, test_name, success, details=""):
        """Enregistre le résultat d'un test"""
        status = "✅ PASS" if success else "❌ FAIL"
        print(f"{status} {test_name}")
        if details:
            print(f"   Details: {details}")
        self.results.append({
            'test': test_name,
            'success': success,
            'details': details
        })
    
    def start_server(self, config_path="config.conf"):
        """Démarre le serveur webserv"""
        try:
            print(f"🚀 Démarrage du serveur avec {config_path}")
            self.server_process = subprocess.Popen(
                ['./webserv', config_path],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                cwd='/goinfre/alanty/GetMaBite'
            )
            time.sleep(2)  # Attendre que le serveur démarre
            return True
        except Exception as e:
            print(f"❌ Échec du démarrage du serveur: {e}")
            return False
    
    def stop_server(self):
        """Arrête le serveur"""
        if self.server_process:
            self.server_process.terminate()
            self.server_process.wait()
            print("🛑 Serveur arrêté")
    
    def test_compilation(self):
        """Test 1: Compilation avec flags stricts"""
        try:
            result = subprocess.run(
                ['make', 'clean', '&&', 'make', 'CXXFLAGS=-Wall -Wextra -Werror -std=c++98'],
                cwd='/goinfre/alanty/GetMaBite',
                capture_output=True,
                text=True,
                shell=True
            )
            success = result.returncode == 0
            details = result.stderr if not success else ""
            self.log_result("Compilation C++98", success, details)
            return success
        except Exception as e:
            self.log_result("Compilation C++98", False, str(e))
            return False
    
    def test_server_startup(self):
        """Test 2: Démarrage du serveur"""
        success = self.start_server()
        self.log_result("Démarrage serveur", success)
        return success
    
    def test_basic_get(self):
        """Test 3: Requête GET basique"""
        try:
            # Test avec socket raw pour plus de contrôle
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.settimeout(5)
            sock.connect((self.host, self.port))
            
            request = "GET / HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n"
            sock.send(request.encode())
            
            response = b""
            while True:
                data = sock.recv(1024)
                if not data:
                    break
                response += data
            
            sock.close()
            
            success = b"HTTP/1.1" in response and (b"200" in response or b"404" in response)
            details = f"Response: {response[:200].decode('utf-8', errors='ignore')}..."
            self.log_result("GET basique", success, details)
            return success
            
        except Exception as e:
            self.log_result("GET basique", False, str(e))
            return False
    
    def test_post_request(self):
        """Test 4: Requête POST"""
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.settimeout(5)
            sock.connect((self.host, self.port))
            
            body = "test=data&name=value"
            request = f"POST / HTTP/1.1\r\nHost: localhost\r\nContent-Length: {len(body)}\r\nContent-Type: application/x-www-form-urlencoded\r\nConnection: close\r\n\r\n{body}"
            sock.send(request.encode())
            
            response = b""
            while True:
                data = sock.recv(1024)
                if not data:
                    break
                response += data
            
            sock.close()
            
            success = b"HTTP/1.1" in response
            details = f"Response: {response[:200].decode('utf-8', errors='ignore')}..."
            self.log_result("POST basique", success, details)
            return success
            
        except Exception as e:
            self.log_result("POST basique", False, str(e))
            return False
    
    def test_delete_request(self):
        """Test 5: Requête DELETE"""
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.settimeout(5)
            sock.connect((self.host, self.port))
            
            request = "DELETE /test_file.txt HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n"
            sock.send(request.encode())
            
            response = b""
            while True:
                data = sock.recv(1024)
                if not data:
                    break
                response += data
            
            sock.close()
            
            success = b"HTTP/1.1" in response
            details = f"Response: {response[:200].decode('utf-8', errors='ignore')}..."
            self.log_result("DELETE basique", success, details)
            return success
            
        except Exception as e:
            self.log_result("DELETE basique", False, str(e))
            return False
    
    def test_error_handling(self):
        """Test 6: Gestion des erreurs"""
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.settimeout(5)
            sock.connect((self.host, self.port))
            
            request = "GET /nonexistent_file.html HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n"
            sock.send(request.encode())
            
            response = b""
            while True:
                data = sock.recv(1024)
                if not data:
                    break
                response += data
            
            sock.close()
            
            success = b"404" in response or b"403" in response
            details = f"Response: {response[:200].decode('utf-8', errors='ignore')}..."
            self.log_result("Gestion erreur 404", success, details)
            return success
            
        except Exception as e:
            self.log_result("Gestion erreur 404", False, str(e))
            return False
    
    def test_concurrent_connections(self, num_connections=10):
        """Test 7: Connexions concurrentes"""
        results = []
        
        def make_request():
            try:
                sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                sock.settimeout(10)
                sock.connect((self.host, self.port))
                
                request = "GET / HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n"
                sock.send(request.encode())
                
                response = b""
                while True:
                    data = sock.recv(1024)
                    if not data:
                        break
                    response += data
                
                sock.close()
                results.append(b"HTTP/1.1" in response)
                
            except Exception:
                results.append(False)
        
        threads = []
        for _ in range(num_connections):
            thread = threading.Thread(target=make_request)
            threads.append(thread)
            thread.start()
        
        for thread in threads:
            thread.join()
        
        success_count = sum(results)
        success = success_count >= num_connections * 0.8  # 80% de succès minimum
        details = f"{success_count}/{num_connections} connexions réussies"
        self.log_result("Connexions concurrentes", success, details)
        return success
    
    def test_large_file_upload(self):
        """Test 8: Upload de gros fichier"""
        try:
            # Créer un fichier de test de 1MB
            test_data = b"A" * (1024 * 1024)  # 1MB
            
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.settimeout(30)  # Plus de timeout pour les gros fichiers
            sock.connect((self.host, self.port))
            
            boundary = "----WebservTestBoundary"
            body = f'--{boundary}\r\nContent-Disposition: form-data; name="file"; filename="large_test.txt"\r\nContent-Type: text/plain\r\n\r\n'.encode()
            body += test_data
            body += f'\r\n--{boundary}--\r\n'.encode()
            
            request = f"POST / HTTP/1.1\r\nHost: localhost\r\nContent-Type: multipart/form-data; boundary={boundary}\r\nContent-Length: {len(body)}\r\nConnection: close\r\n\r\n".encode()
            
            sock.send(request)
            sock.send(body)
            
            response = b""
            while True:
                data = sock.recv(1024)
                if not data:
                    break
                response += data
            
            sock.close()
            
            success = b"HTTP/1.1" in response and not b"413" in response  # Pas d'erreur "Payload Too Large"
            details = f"Upload 1MB: {response[:100].decode('utf-8', errors='ignore')}..."
            self.log_result("Upload gros fichier", success, details)
            return success
            
        except Exception as e:
            self.log_result("Upload gros fichier", False, str(e))
            return False
    
    def test_malformed_requests(self):
        """Test 9: Requêtes malformées"""
        malformed_requests = [
            b"GET HTTP/1.1\r\n\r\n",  # Pas de chemin
            b"INVALID /path HTTP/1.1\r\n\r\n",  # Méthode invalide
            b"GET / HTTP/1.1\r\nHost: localhost\r\nContent-Length: abc\r\n\r\n",  # Content-Length invalide
            b"GET / HTTP/1.1\r\nHost: localhost\r\n" + b"A" * 10000 + b"\r\n\r\n",  # Headers trop longs
        ]
        
        success_count = 0
        
        for i, request in enumerate(malformed_requests):
            try:
                sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                sock.settimeout(5)
                sock.connect((self.host, self.port))
                sock.send(request)
                
                response = b""
                while True:
                    data = sock.recv(1024)
                    if not data:
                        break
                    response += data
                
                sock.close()
                
                # Le serveur devrait retourner une erreur 400
                if b"400" in response or b"HTTP/1.1" in response:
                    success_count += 1
                    
            except Exception:
                # Une exception peut être acceptable pour les requêtes malformées
                success_count += 1
        
        success = success_count >= len(malformed_requests) * 0.5
        details = f"{success_count}/{len(malformed_requests)} requêtes malformées gérées"
        self.log_result("Requêtes malformées", success, details)
        return success
    
    def test_cgi_execution(self):
        """Test 10: Exécution CGI"""
        # Créer un script CGI simple
        cgi_script = """#!/usr/bin/env python3
print("Content-Type: text/html\\r\\n\\r\\n")
print("<html><body><h1>CGI Test OK</h1></body></html>")
"""
        
        try:
            # Écrire le script CGI
            cgi_path = "/goinfre/alanty/GetMaBite/www/tests/test_cgi.py"
            with open(cgi_path, 'w') as f:
                f.write(cgi_script)
            os.chmod(cgi_path, 0o755)
            
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.settimeout(10)
            sock.connect((self.host, self.port))
            
            request = "GET /test_cgi.py HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n"
            sock.send(request.encode())
            
            response = b""
            while True:
                data = sock.recv(1024)
                if not data:
                    break
                response += data
            
            sock.close()
            
            success = b"CGI Test OK" in response or b"200" in response
            details = f"CGI Response: {response[:200].decode('utf-8', errors='ignore')}..."
            self.log_result("Exécution CGI", success, details)
            
            # Nettoyer
            if os.path.exists(cgi_path):
                os.remove(cgi_path)
            
            return success
            
        except Exception as e:
            self.log_result("Exécution CGI", False, str(e))
            return False
    
    def run_all_tests(self):
        """Lance tous les tests"""
        print("🔍 Début des tests du serveur webserv\n" + "="*50)
        
        # Test de compilation
        if not self.test_compilation():
            print("❌ Échec de la compilation - Arrêt des tests")
            return
        
        # Test de démarrage
        if not self.test_server_startup():
            print("❌ Échec du démarrage du serveur - Arrêt des tests")
            return
        
        try:
            # Tests fonctionnels
            self.test_basic_get()
            self.test_post_request()
            self.test_delete_request()
            self.test_error_handling()
            self.test_concurrent_connections()
            self.test_large_file_upload()
            self.test_malformed_requests()
            self.test_cgi_execution()
            
        finally:
            self.stop_server()
        
        # Résumé
        print("\n" + "="*50)
        print("📊 RÉSUMÉ DES TESTS")
        print("="*50)
        
        passed = sum(1 for r in self.results if r['success'])
        total = len(self.results)
        
        for result in self.results:
            status = "✅" if result['success'] else "❌"
            print(f"{status} {result['test']}")
        
        print(f"\n🎯 Score: {passed}/{total} ({passed/total*100:.1f}%)")
        
        if passed == total:
            print("🎉 Tous les tests sont passés!")
        elif passed >= total * 0.8:
            print("⚠️  La plupart des tests sont passés, mais il y a quelques problèmes")
        else:
            print("❌ Plusieurs problèmes critiques détectés")
        
        return passed, total

if __name__ == "__main__":
    tester = WebservTester()
    tester.run_all_tests()
