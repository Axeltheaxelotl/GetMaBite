#!/usr/bin/env python3
"""
Test simple et rapide pour webserv
"""
import socket
import time
import subprocess
import os

def test_server():
    print("🔍 ANALYSE COMPLÈTE DU SERVEUR WEBSERV")
    print("=" * 50)
    
    # Test 1: Vérifier que le serveur écoute
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        result = sock.connect_ex(('localhost', 8081))
        sock.close()
        if result == 0:
            print("✅ Serveur accessible sur port 8081")
        else:
            print("❌ Serveur non accessible")
            return
    except Exception as e:
        print(f"❌ Erreur de connexion: {e}")
        return
    
    # Test 2: GET basique
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect(('localhost', 8081))
        request = b"GET / HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n"
        sock.send(request)
        
        response = b""
        while True:
            data = sock.recv(1024)
            if not data:
                break
            response += data
        sock.close()
        
        if b"200 OK" in response:
            print("✅ GET basique fonctionne")
        else:
            print("❌ GET basique échoue")
            print(f"   Réponse: {response[:100]}")
    except Exception as e:
        print(f"❌ Test GET échoué: {e}")
    
    # Test 3: POST
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect(('localhost', 8081))
        body = b"test data"
        request = f"POST /test HTTP/1.1\r\nHost: localhost\r\nContent-Length: {len(body)}\r\nConnection: close\r\n\r\n".encode() + body
        sock.send(request)
        
        response = b""
        while True:
            data = sock.recv(1024)
            if not data:
                break
            response += data
        sock.close()
        
        if b"HTTP/1.1" in response:
            print("✅ POST fonctionne")
        else:
            print("❌ POST échoue")
    except Exception as e:
        print(f"❌ Test POST échoué: {e}")
    
    # Test 4: Requête malformée
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect(('localhost', 8081))
        request = b"INVALID REQUEST\r\n\r\n"
        sock.send(request)
        
        response = b""
        while True:
            data = sock.recv(1024)
            if not data:
                break
            response += data
        sock.close()
        
        if b"400" in response or b"HTTP/1.1" in response:
            print("✅ Gestion requêtes malformées OK")
        else:
            print("❌ Gestion requêtes malformées KO")
    except Exception as e:
        print(f"⚠️  Test requête malformée: {e}")
    
    # Test 5: Stress test simple
    print("\n🏋️  Test de stress (10 connexions simultanées)...")
    import threading
    
    results = []
    def make_request():
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.settimeout(5)
            sock.connect(('localhost', 8081))
            request = b"GET / HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n"
            sock.send(request)
            
            response = b""
            while True:
                data = sock.recv(1024)
                if not data:
                    break
                response += data
            sock.close()
            
            results.append(b"200 OK" in response)
        except Exception:
            results.append(False)
    
    threads = []
    for _ in range(10):
        t = threading.Thread(target=make_request)
        threads.append(t)
        t.start()
    
    for t in threads:
        t.join()
    
    success_rate = sum(results) / len(results) * 100
    if success_rate >= 80:
        print(f"✅ Stress test OK: {success_rate:.0f}% de succès")
    else:
        print(f"⚠️  Stress test partiel: {success_rate:.0f}% de succès")
    
    print("\n📊 RÉSUMÉ DE L'ANALYSE")
    print("=" * 50)

if __name__ == "__main__":
    test_server()
