#!/usr/bin/env python3
"""
Test spécifique pour YoupiBanane
"""
import socket
import subprocess
import time

def test_youpie_banana():
    print("🍌 TEST YOUPIE BANANA 🍌")
    print("=" * 40)
    
    # Démarrer le serveur
    print("Démarrage du serveur...")
    server_process = subprocess.Popen(['./webserv', 'config.conf'], 
                                    stdout=subprocess.PIPE, 
                                    stderr=subprocess.PIPE)
    time.sleep(3)
    
    try:
        tests = [
            ("/directory/", "Test via /directory/"),
            ("/YoupiBanane/", "Test via /YoupiBanane/"),
            ("/directory/nop/", "Test sous-dossier nop"),
            ("/directory/Yeah/", "Test sous-dossier Yeah")
        ]
        
        for path, description in tests:
            print(f"\n{description}:")
            try:
                sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                sock.settimeout(5)
                sock.connect(('localhost', 8081))
                
                request = f"GET {path} HTTP/1.1\r\n"
                request += "Host: localhost:8081\r\n\r\n"
                
                sock.sendall(request.encode())
                response = sock.recv(4096).decode()
                sock.close()
                
                if "200" in response:
                    if "youpi.bla" in response or "Index of" in response:
                        print(f"  ✅ SUCCÈS - Autoindex fonctionne")
                        print(f"  📁 Contenu détecté dans la réponse")
                    else:
                        print(f"  ⚠️  Code 200 mais pas d'autoindex visible")
                else:
                    print(f"  ❌ ÉCHEC - Status: {response[:100]}")
                    
            except Exception as e:
                print(f"  ❌ ERREUR - {str(e)}")
        
    finally:
        print("\nArrêt du serveur...")
        server_process.terminate()
        try:
            server_process.wait(timeout=5)
        except subprocess.TimeoutExpired:
            server_process.kill()
    
    print("\n🎉 Test YoupiBanane terminé!")

if __name__ == "__main__":
    test_youpie_banana()
