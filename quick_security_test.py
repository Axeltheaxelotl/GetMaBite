#!/usr/bin/env python3
"""
Test rapide et simple pour valider les corrections de sécurité
"""

import socket
import time
import subprocess
import os
import signal

def test_path_traversal():
    """Test simple de path traversal"""
    print("🔒 Test Path Traversal...")
    
    # Démarrer le serveur
    server = subprocess.Popen(
        ["./webserv", "config.conf"],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE
    )
    
    time.sleep(3)  # Attendre le démarrage
    
    try:
        # Test d'attaque path traversal
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(5)
        sock.connect(("localhost", 8081))
        
        # Tentative d'accès à /etc/passwd
        attack_request = "GET /../../../etc/passwd HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n"
        sock.send(attack_request.encode())
        
        response = sock.recv(4096).decode()
        sock.close()
        
        # Vérifier que l'attaque est bloquée
        if "root:" not in response and ("404" in response or "403" in response or "400" in response):
            print("✅ Path traversal BLOQUÉ - Sécurité OK")
            return True
        else:
            print("❌ Path traversal NON BLOQUÉ - Problème de sécurité")
            print(f"Réponse: {response[:200]}...")
            return False
            
    except Exception as e:
        print(f"❌ Erreur lors du test: {e}")
        return False
    finally:
        server.terminate()
        server.wait()

def test_buffer_size():
    """Test de la gestion des gros buffers"""
    print("🛡️ Test Buffer Size...")
    
    server = subprocess.Popen(
        ["./webserv", "config.conf"],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE
    )
    
    time.sleep(3)
    
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(10)
        sock.connect(("localhost", 8081))
        
        # Test avec un gros body (20KB)
        large_body = "A" * 20480
        request = f"POST /test HTTP/1.1\r\nHost: localhost\r\nContent-Length: {len(large_body)}\r\nConnection: close\r\n\r\n{large_body}"
        
        sock.send(request.encode())
        response = sock.recv(4096).decode()
        sock.close()
        
        if "HTTP/" in response:
            print("✅ Gros buffers gérés correctement")
            return True
        else:
            print("❌ Problème avec les gros buffers")
            return False
            
    except Exception as e:
        print(f"❌ Erreur lors du test buffer: {e}")
        return False
    finally:
        server.terminate()
        server.wait()

def main():
    print("🚀 TESTS DE VALIDATION DES CORRECTIONS")
    print("=" * 50)
    
    if not os.path.exists("./webserv"):
        print("❌ Serveur webserv introuvable")
        return
    
    results = []
    results.append(test_path_traversal())
    results.append(test_buffer_size())
    
    print("\n📊 RÉSULTATS")
    print("=" * 50)
    passed = sum(results)
    total = len(results)
    
    print(f"Tests réussis: {passed}/{total}")
    
    if passed == total:
        print("🎉 TOUTES LES CORRECTIONS FONCTIONNENT!")
    else:
        print("⚠️ Certaines corrections nécessitent plus de travail")

if __name__ == "__main__":
    main()
