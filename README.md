# Webserv - Guide d'Évaluation 42

## 📋 Introduction

Ce projet implémente un serveur HTTP en C++98 conforme au standard HTTP/1.1. Le serveur utilise une architecture non-bloquante basée sur `epoll` pour gérer de multiples connexions simultanées.

---

## 🔧 Compilation et Lancement

```bash
# Compilation
make

# Nettoyage
make clean
make fclean

# Relancement
make re

# Lancement du serveur
./webserv [fichier_configuration]
./webserv config.conf
```

---

## 📖 Partie Obligatoire

### 1. Vérification du Code et Questions

#### Questions Techniques Fondamentales
- **Bases du serveur HTTP** : Notre serveur implémente HTTP/1.1 avec support des méthodes GET, POST, DELETE
- **Multiplexage I/O** : Utilisation d'`epoll` (système Linux) pour la gestion asynchrone des événements
- **Architecture non-bloquante** : Un seul `epoll_wait()` dans la boucle principale de `EpollClasse::serverRun()`

#### Points Critiques Vérifiés
✅ **Un seul select/epoll** : Toutes les opérations I/O passent par `EpollClasse::serverRun()`  
✅ **Pas de read/write direct** : Toutes les opérations utilisent l'epoll  
✅ **Gestion d'erreurs** : Vérification des valeurs de retour (-1 ET 0)  
✅ **Pas de vérification errno** : Conformément aux exigences  

#### Localisation du Code Principal
- **Boucle principale** : `src/core/EpollClasse.cpp` - méthode `serverRun()`
- **Gestion epoll** : `src/core/EpollClasse.hpp` et `.cpp`
- **Configuration** : `src/config/Parser.cpp`

---

### 2. Configuration

#### Tests de Configuration Disponibles

**Serveurs multiples - Ports différents :**
```bash
./webserv config/good/multipleServer.conf
curl http://localhost:8080/
curl http://localhost:8081/
```

**Serveurs multiples - Noms d'hôte différents :**
```bash
./webserv config/good/multipleServerName.conf
curl --resolve example.com:8080:127.0.0.1 http://example.com:8080/
curl --resolve test.com:8080:127.0.0.1 http://test.com:8080/
```

**Pages d'erreur personnalisées :**
```bash
./webserv config/good/customError.conf
curl -i http://localhost:8080/page_inexistante  # 404 personnalisé
```

**Limite taille du corps :**
```bash
./webserv config/good/maxBodySize.conf
# Corps trop grand
curl -X POST -H "Content-Type: text/plain" \
  --data "$(yes 'A' | head -n 10000 | tr -d '\n')" \
  http://localhost:8080/upload

# Corps normal
curl -X POST -H "Content-Type: text/plain" \
  --data "petit corps" \
  http://localhost:8080/upload
```

**Méthodes autorisées :**
```bash
./webserv config/good/allowMethod.conf
curl -X GET http://localhost:8080/     # Autorisé
curl -X POST http://localhost:8080/    # Autorisé
curl -X DELETE http://localhost:8080/  # Selon config
curl -X PUT http://localhost:8080/     # Non autorisé
```

**Routes et répertoires :**
```bash
./webserv config/good/default.conf
curl http://localhost:8080/tests/      # Route spécifique
curl http://localhost:8080/           # Route par défaut
```

**Fichier d'index par défaut :**
```bash
./webserv config/good/default.conf
curl http://localhost:8080/           # Affiche index.html
```

---

### 3. Contrôles de Base

#### Tests HTTP Fondamentaux

**GET :**
```bash
curl -i http://localhost:8080/
curl -i http://localhost:8080/tests/index.html
```

**POST :**
```bash
curl -X POST -d "test=value&name=webserv" http://localhost:8080/
curl -X POST -F "file=@test.txt" http://localhost:8080/upload/
```

**DELETE :**
```bash
echo "test" > www/tests/delete_me.txt
curl -X DELETE http://localhost:8080/tests/delete_me.txt
```

**Méthodes inconnues (ne doit pas crasher) :**
```bash
curl -X UNKNOWN http://localhost:8080/
curl -X PATCH http://localhost:8080/
```

**Upload et récupération :**
```bash
# Créer un fichier test
echo "contenu test" > test_upload.txt

# Upload
curl -X POST -F "file=@test_upload.txt" http://localhost:8080/upload/

# Vérification
curl http://localhost:8080/upload/test_upload.txt
```

---

### 4. Vérification CGI

#### Scripts de Test Disponibles

Le serveur supporte les CGI dans le répertoire `www/tests/` :

**Test CGI Python (GET) :**
```bash
curl http://localhost:8080/tests/simple.cgi.py
```

**Test CGI Python (POST) :**
```bash
curl -X POST -d "name=test&value=42" \
  http://localhost:8080/tests/form_processor.cgi.py
```

**Test CGI Shell :**
```bash
curl http://localhost:8080/tests/hello.cgi.sh
```

**Test environnement CGI :**
```bash
curl http://localhost:8080/tests/cgi_env.py
```

**Test gestion d'erreurs CGI :**
```bash
curl -i http://localhost:8080/tests/error_test.cgi.py
```

**Test stderr CGI :**
```bash
curl -i http://localhost:8080/tests/stderr.cgi.py
```

#### Points de Vérification CGI
✅ **Répertoire d'exécution correct** : Les CGI s'exécutent dans le bon répertoire  
✅ **Variables d'environnement** : REQUEST_METHOD, QUERY_STRING, etc.  
✅ **Gestion d'erreurs** : Scripts avec erreurs gérés proprement  
✅ **Timeout** : Scripts infinis interrompus correctement  

---

### 5. Vérification avec Navigateur

#### Tests Manuels
1. **Ouvrir navigateur** : `http://localhost:8080`
2. **F12** → Onglet Network
3. **Tester** :
   - Page normale : `http://localhost:8080/tests/`
   - URL inexistante : `http://localhost:8080/inexistant` (404)
   - Listage répertoire : `http://localhost:8080/tests/` (si autoindex activé)
   - Redirection : selon configuration
   - Fichiers statiques : CSS, images dans `www/`

#### Test avec telnet
```bash
telnet localhost 8080
GET /tests/ HTTP/1.1
Host: localhost
Connection: close

# (Appuyer Entrée deux fois)
```

---

### 6. Questions relatives au Port

#### Configuration Ports Multiples
```bash
# Lancer serveur normal
./webserv config/good/multipleServer.conf

# Tester ports différents
curl http://localhost:8080/
curl http://localhost:8081/
```

#### Test Ports Identiques (doit échouer)
```bash
# Terminal 1
./webserv config.conf

# Terminal 2 (doit échouer)
./webserv config.conf
```

#### Vérification Configuration
```bash
# Vérifier ports en écoute
ss -tuln | grep :8080
netstat -an | grep :8080
```

---

### 7. Test de Résistance avec Siege

#### Installation Siege
```bash
# Ubuntu/Debian
sudo apt-get install siege

# macOS
brew install siege
```

#### Tests de Performance
```bash
# Test basique
siege -c 10 -t 30s http://localhost:8080/

# Test intensif
siege -c 50 -t 60s http://localhost:8080/

# Test availability (DOIT être > 99.5%)
siege -c 20 -t 30s http://localhost:8080/tests/

# Test prolongé
siege -c 25 -t 300s http://localhost:8080/
```

#### Monitoring Mémoire
```bash
# Terminal 1: Lancer siege
siege -c 50 -t 300s http://localhost:8080/

# Terminal 2: Monitor mémoire
watch -n 1 'ps aux | grep webserv | grep -v grep'

# Vérification fuites mémoire
valgrind --leak-check=full ./webserv config.conf
```

#### Vérification Connexions
```bash
# Connexions actives
ss -tuln | grep :8080

# Connexions pendantes
netstat -an | grep :8080 | grep TIME_WAIT
```

---

## 🎁 Partie Bonus

### 1. Cookies et Sessions

#### Tests disponibles
```bash
# Test cookies (si implémenté)
curl -c cookies.txt -b cookies.txt http://localhost:8080/tests/simple_session.cgi.py

# Vérifier headers Set-Cookie
curl -I http://localhost:8080/login
```

### 2. CGI Multiples

#### Langages supportés
- **Python** : `*.cgi.py`
- **Shell** : `*.cgi.sh`  
- **Perl** : `*.cgi.pl`
- **Ruby** : `*.cgi.rb`
- **JavaScript/Node** : `*.cgi.js`
- **AWK** : `*.cgi.awk`
- **C compilé** : `*.cgi`

#### Tests
```bash
curl http://localhost:8080/tests/simple.cgi.py      # Python
curl http://localhost:8080/tests/hello.cgi.sh       # Shell
curl http://localhost:8080/tests/perl_demo.cgi.pl   # Perl
curl http://localhost:8080/tests/ruby_gem.cgi.rb    # Ruby
curl http://localhost:8080/tests/node_rocket.cgi.js # Node.js
```

---

## ⚠️ Points Critiques d'Évaluation

### Erreurs qui donnent 0 points
- [ ] **errno vérifié** après read/write/recv/send
- [ ] **Pas d'epoll** ou plusieurs boucles d'attente
- [ ] **read/write direct** sans epoll
- [ ] **Crash** avec requête invalide
- [ ] **Erreurs de compilation**
- [ ] **Availability < 99.5%** avec siege

### Points obligatoires
- [x] Compilation sans erreur
- [x] Un seul epoll dans boucle principale
- [x] Pas de read/write direct sans epoll
- [x] Gestion erreurs correcte (pas errno)
- [x] Configuration multiples serveurs
- [x] GET, POST, DELETE fonctionnent
- [x] CGI fonctionne (GET et POST)
- [x] Pas de crash avec requêtes invalides
- [x] Compatible navigateur web
- [x] Tests siege > 99.5% availability

---

## 🗂️ Structure des Fichiers

```
src/
├── main.cpp                    # Point d'entrée
├── core/
│   ├── EpollClasse.*          # Boucle principale epoll
│   └── TimeoutManager.*       # Gestion timeouts
├── config/
│   ├── Parser.*               # Parseur configuration
│   ├── Server.*               # Structure serveur
│   └── Location.*             # Structure location
├── http/
│   ├── Cookie.*               # Gestion cookies (bonus)
│   └── RequestBufferManager.* # Gestion buffers requêtes
└── cgi/
    └── CgiHandler.*           # Gestion CGI

config/
├── good/                      # Configurations valides
└── bad/                       # Configurations invalides (tests)

www/
├── tests/                     # Scripts CGI de test
└── errors/                    # Pages d'erreur personnalisées
```

---

## 🔍 Commandes de Debug

```bash
# Logs en temps réel
tail -f /var/log/webserv.log

# Vérification processus
ps aux | grep webserv

# Connexions réseau
lsof -i :8080

# Test configuration
./webserv -t config.conf
```

---

## 📞 Support Évaluation

Ce serveur a été testé et vérifié selon tous les critères du sujet webserv de 42. Tous les scripts de test sont disponibles dans le répertoire `www/tests/` et les configurations dans `config/good/`.

**Availability confirmée** : > 99.5% avec siege  
**Architecture** : Non-bloquante avec epoll unique  
**Conformité** : HTTP/1.1 standard  
**CGI** : Support multi-langages  
