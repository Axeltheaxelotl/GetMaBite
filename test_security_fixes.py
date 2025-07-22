#!/usr/bin/env python3
"""
Test de validation des corrections de sécurité pour webserv
Teste spécifiquement les correctifs appliqués
"""

import socket
import time
import threading
import subprocess
import os
import signal

class SecurityTester:
    def __init__(self, host="localhost", port=8081):
        self.host = host
        self.port = port
        self.server_process = None
        self.results = []

    def log_test(self, test_name, success, details=""):
        status = "✅ PASS" if success else "❌ FAIL"
        print(f"{status} {test_name}")
        if details:
            print(f"    Details: {details}")
        self.results.append({'test': test_name, 'success': success, 'details': details})

    def start_server(self):
        """Démarre le serveur webserv en arrière-plan"""
        try:
            self.server_process = subprocess.Popen(
                ["./webserv", "config.conf"],
                stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL
            )
            time.sleep(2)  # Attendre le démarrage
            return True
        except Exception as e:
            print(f"❌ Impossible de démarrer le serveur: {e}")
            return False

    def stop_server(self):
        """Arrête le serveur"""
        if self.server_process:
            self.server_process.terminate()
            self.server_process.wait()

    def send_raw_request(self, request):
        """Envoie une requête HTTP brute et retourne la réponse"""
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.settimeout(5)
            sock.connect((self.host, self.port))
            sock.send(request.encode() if isinstance(request, str) else request)
            
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

    def test_path_traversal_blocked(self):
        """Test 1: Vérification que path traversal est bloqué"""
        attack_paths = [
            "/../../../etc/passwd",
            "/../../etc/hosts", 
            "/../../../../../boot/grub/grub.cfg",
            "/tests/../../../etc/passwd",
            "/www/../../../etc/shadow"
        ]
        
        blocked_count = 0
        for path in attack_paths:
            request = f"GET {path} HTTP/1.1\r\nHost: {self.host}\r\nConnection: close\r\n\r\n"
            response = self.send_raw_request(request)
            
            # Vérifier que la réponse ne contient pas de contenu système sensible
            if ("root:" not in response and 
                "daemon:" not in response and 
                ("404" in response or "403" in response or "400" in response)):
                blocked_count += 1
                
        success = blocked_count == len(attack_paths)
        self.log_test("Path Traversal Protection", success, 
                     f"{blocked_count}/{len(attack_paths)} attaques bloquées")

    def test_large_buffer_handling(self):
        """Test 2: Gestion des gros buffers (correction buffer overflow)"""
        try:
            # Créer un gros body de test (100KB)
            large_body = "A" * 102400
            request = (f"POST /test_large HTTP/1.1\r\n"
                      f"Host: {self.host}\r\n"
                      f"Content-Length: {len(large_body)}\r\n"
                      f"Connection: close\r\n\r\n{large_body}")
            
            response = self.send_raw_request(request)
            
            # Le serveur ne doit pas crasher et doit répondre
            success = "HTTP/" in response and len(response) > 0
            status = "201" if "201" in response else ("413" if "413" in response else "other")
            
            self.log_test("Large Buffer Handling", success, 
                         f"Serveur répond avec status: {status}")
        except Exception as e:
            self.log_test("Large Buffer Handling", False, f"Exception: {str(e)}")

    def test_server_stability_under_load(self):
        """Test 3: Stabilité du serveur avec timeout optimisé"""
        def make_concurrent_request():
            request = f"GET / HTTP/1.1\r\nHost: {self.host}\r\nConnection: close\r\n\r\n"
            response = self.send_raw_request(request)
            return "200 OK" in response
            
        # Test avec 20 connexions simultanées
        threads = []
        results = []
        
        for _ in range(20):
            t = threading.Thread(target=lambda: results.append(make_concurrent_request()))
            threads.append(t)
            t.start()
            
        for t in threads:
            t.join()
            
        success_rate = sum(results) / len(results) if results else 0
        success = success_rate >= 0.9  # 90% de succès minimum
        
        self.log_test("Server Stability Under Load", success, 
                     f"Taux de succès: {success_rate*100:.1f}%")

    def test_malformed_requests(self):
        """Test 4: Gestion des requêtes malformées"""
        malformed_requests = [
            "INVALID REQUEST\r\n\r\n",
            "GET \r\n\r\n",  # Path manquant
            "GET / HTTP/\r\n\r\n",  # Version HTTP incomplète
            "GET /" + "\0" + " HTTP/1.1\r\n\r\n",  # Caractère null
            "GET //" + "A"*1000 + " HTTP/1.1\r\n\r\n"  # Path très long
        ]
        
        handled_count = 0
        for req in malformed_requests:
            response = self.send_raw_request(req)
            # Le serveur doit répondre avec une erreur ou fermer proprement
            if ("400" in response or "HTTP/" in response or len(response) == 0):
                handled_count += 1
                
        success = handled_count == len(malformed_requests)
        self.log_test("Malformed Requests Handling", success,
                     f"{handled_count}/{len(malformed_requests)} requêtes gérées")

    def test_memory_safety(self):
        """Test 5: Test de sécurité mémoire basique"""
        # Envoyer plusieurs requêtes avec différentes tailles pour tester les buffers
        sizes = [100, 1000, 10000, 50000]
        all_handled = True
        
        for size in sizes:
            body = "B" * size
            request = (f"POST /memory_test HTTP/1.1\r\n"
                      f"Host: {self.host}\r\n" 
                      f"Content-Length: {len(body)}\r\n"
                      f"Connection: close\r\n\r\n{body}")
            
            response = self.send_raw_request(request)
            if not response or "HTTP/" not in response:
                all_handled = False
                break
                
        self.log_test("Memory Safety", all_handled, 
                     "Tailles testées: " + str(sizes))

    def run_all_tests(self):
        """Exécute tous les tests de validation"""
        print("🔒 VALIDATION DES CORRECTIONS DE SÉCURITÉ WEBSERV")
        print("=" * 60)
        
        if not self.start_server():
            return
            
        try:
            self.test_path_traversal_blocked()
            self.test_large_buffer_handling()
            self.test_server_stability_under_load()
            self.test_malformed_requests()
            self.test_memory_safety()
            
        finally:
            self.stop_server()
            
        # Résumé
        passed = sum(1 for r in self.results if r['success'])
        total = len(self.results)
        
        print("\n📊 RÉSULTATS FINAUX")
        print("=" * 60)
        print(f"Tests passés: {passed}/{total}")
        print(f"Taux de succès: {passed/total*100:.1f}%")
        
        if passed == total:
            print("🎉 TOUTES LES CORRECTIONS FONCTIONNENT PARFAITEMENT !")
        else:
            print("⚠️  Certaines corrections nécessitent une attention supplémentaire")

if __name__ == "__main__":
    tester = SecurityTester()
    tester.run_all_tests()
