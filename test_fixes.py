#!/usr/bin/env python3
"""
Test rapide pour vérifier les corrections du max_body_size et YoupiBanane
"""
import socket
import time
import subprocess
import os
import signal

def test_large_body():
    """Test du Large Body avec la nouvelle limite de 10MB"""
    print("Test 1: Large Body (5MB)...")
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(10)
        sock.connect(('localhost', 8081))
        
        # Créer un corps de 5MB (sous la limite de 10MB)
        large_body = "A" * (5 * 1024 * 1024)  # 5MB
        
        request = "POST /test_large HTTP/1.1\r\n"
        request += "Host: localhost:8081\r\n"
        request += f"Content-Length: {len(large_body)}\r\n"
        request += "Content-Type: text/plain\r\n\r\n"
        request += large_body
        
        sock.sendall(request.encode())
        response = sock.recv(4096).decode()
        sock.close()
        
        if "201" in response or "200" in response:
            print("✅ Large Body (5MB): PASSED")
            return True
        else:
            print(f"❌ Large Body (5MB): FAILED - Response: {response[:100]}...")
            return False
            
    except Exception as e:
        print(f"❌ Large Body (5MB): ERROR - {str(e)}")
        return False

def test_youpi_banane():
    """Test de l'autoindex du répertoire YoupiBanane"""
    print("Test 2: YoupiBanane autoindex...")
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(5)
        sock.connect(('localhost', 8081))
        
        request = "GET /YoupiBanane/ HTTP/1.1\r\n"
        request += "Host: localhost:8081\r\n\r\n"
        
        sock.sendall(request.encode())
        response = sock.recv(4096).decode()
        sock.close()
        
        if "200" in response and ("youpi.bla" in response or "Index of" in response):
            print("✅ YoupiBanane autoindex: PASSED")
            return True
        else:
            print(f"❌ YoupiBanane autoindex: FAILED - Response: {response[:200]}...")
            return False
            
    except Exception as e:
        print(f"❌ YoupiBanane autoindex: ERROR - {str(e)}")
        return False

def test_directory_autoindex():
    """Test de l'autoindex du répertoire /directory/"""
    print("Test 3: /directory/ autoindex...")
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(5)
        sock.connect(('localhost', 8081))
        
        request = "GET /directory/ HTTP/1.1\r\n"
        request += "Host: localhost:8081\r\n\r\n"
        
        sock.sendall(request.encode())
        response = sock.recv(4096).decode()
        sock.close()
        
        if "200" in response and ("youpi.bla" in response or "Index of" in response):
            print("✅ /directory/ autoindex: PASSED")
            return True
        else:
            print(f"❌ /directory/ autoindex: FAILED - Response: {response[:200]}...")
            return False
            
    except Exception as e:
        print(f"❌ /directory/ autoindex: ERROR - {str(e)}")
        return False

def main():
    print("=" * 60)
    print("TEST DES CORRECTIONS - MAX_BODY_SIZE & YOUPIBANANE")
    print("=" * 60)
    
    # Démarrer le serveur
    print("Démarrage du serveur webserv...")
    server_process = subprocess.Popen(['./webserv', 'config.conf'], 
                                    stdout=subprocess.PIPE, 
                                    stderr=subprocess.PIPE)
    time.sleep(3)  # Attendre que le serveur démarre
    
    try:
        results = []
        
        # Tests
        results.append(test_large_body())
        results.append(test_youpi_banane())
        results.append(test_directory_autoindex())
        
        # Résultats
        passed = sum(results)
        total = len(results)
        
        print("\n" + "=" * 60)
        print("RÉSULTATS")
        print("=" * 60)
        print(f"Tests réussis: {passed}/{total}")
        
        if passed == total:
            print("🎉 TOUS LES TESTS SONT PASSÉS!")
        else:
            print("⚠️  Certains tests ont échoué")
        
    finally:
        # Arrêter le serveur
        print("\nArrêt du serveur...")
        server_process.terminate()
        try:
            server_process.wait(timeout=5)
        except subprocess.TimeoutExpired:
            server_process.kill()

if __name__ == "__main__":
    main()
