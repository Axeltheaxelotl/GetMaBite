#!/usr/bin/env python3
import socket
import time
import subprocess
import signal
import sys

def test_server_functionality():
    """Test rapide des fonctionnalités corrigées"""
    
    # Démarrer le serveur
    print("Démarrage du serveur webserv...")
    server_process = subprocess.Popen(['./webserv', 'config.conf'], 
                                    stdout=subprocess.PIPE, 
                                    stderr=subprocess.PIPE)
    time.sleep(2)  # Attendre que le serveur démarre
    
    try:
        results = {}
        
        # Test 1: POST Method
        print("Test 1: POST Method...")
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.settimeout(5)
            sock.connect(('localhost', 8081))
            
            request = "POST / HTTP/1.1\r\n"
            request += "Host: localhost:8081\r\n"
            request += "Content-Type: application/x-www-form-urlencoded\r\n"
            request += "Content-Length: 13\r\n\r\n"
            request += "test=postdata"
            
            sock.sendall(request.encode())
            response = sock.recv(4096).decode()
            sock.close()
            
            if "201" in response or "200" in response:
                results['POST'] = "PASS"
            else:
                results['POST'] = f"FAIL - {response[:100]}"
                
        except Exception as e:
            results['POST'] = f"ERROR - {str(e)}"
        
        # Test 2: Autoindex
        print("Test 2: Autoindex...")
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.settimeout(5)
            sock.connect(('localhost', 8081))
            
            request = "GET /directory/ HTTP/1.1\r\n"
            request += "Host: localhost:8081\r\n\r\n"
            
            sock.sendall(request.encode())
            response = sock.recv(4096).decode()
            sock.close()
            
            if "200" in response and ("Index of" in response or "<ul>" in response):
                results['Autoindex'] = "PASS"
            else:
                results['Autoindex'] = f"FAIL - {response[:100]}"
                
        except Exception as e:
            results['Autoindex'] = f"ERROR - {str(e)}"
        
        # Test 3: File Upload
        print("Test 3: File Upload...")
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.settimeout(5)
            sock.connect(('localhost', 8081))
            
            boundary = "----TestBoundary"
            body = f"--{boundary}\r\n"
            body += 'Content-Disposition: form-data; name="file"; filename="test.txt"\r\n'
            body += "Content-Type: text/plain\r\n\r\n"
            body += "Test file content"
            body += f"\r\n--{boundary}--\r\n"
            
            request = "POST / HTTP/1.1\r\n"
            request += "Host: localhost:8081\r\n"
            request += f"Content-Type: multipart/form-data; boundary={boundary}\r\n"
            request += f"Content-Length: {len(body)}\r\n\r\n"
            request += body
            
            sock.sendall(request.encode())
            response = sock.recv(4096).decode()
            sock.close()
            
            if "200" in response or "201" in response:
                results['FileUpload'] = "PASS"
            else:
                results['FileUpload'] = f"FAIL - {response[:100]}"
                
        except Exception as e:
            results['FileUpload'] = f"ERROR - {str(e)}"
        
        # Afficher les résultats
        print("\n" + "="*50)
        print("RÉSULTATS DES TESTS RAPIDES")
        print("="*50)
        for test, result in results.items():
            status = "✅" if result == "PASS" else "❌"
            print(f"{status} {test}: {result}")
        
        passed = sum(1 for r in results.values() if r == "PASS")
        total = len(results)
        print(f"\nRésultat: {passed}/{total} tests réussis")
        
    finally:
        # Arrêter le serveur
        print("\nArrêt du serveur...")
        server_process.terminate()
        try:
            server_process.wait(timeout=5)
        except subprocess.TimeoutExpired:
            server_process.kill()

if __name__ == "__main__":
    test_server_functionality()
