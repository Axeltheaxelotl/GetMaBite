# Webserv
C'est le moment de comprendre pourquoi les URLs commencent par HTTP !

## Résumé
Ce projet consiste à écrire votre propre serveur HTTP.
Vous allez pouvoir le tester dans un véritable navigateur.
HTTP est le protocole le plus utilisé sur internet, il est l'heure d'en connaitre les arcanes.

**Version:** 21.2

---

## Table des matières
- [I. Introduction](#introduction)
- [II. Consignes générales](#consignes-générales)
- [III. Partie obligatoire](#partie-obligatoire)
- [IV. Compilation et Utilisation](#compilation-et-utilisation)
- [V. Tests et Évaluation](#tests-et-évaluation)
- [VI. Guide Technique](#guide-technique)

---

## I. Introduction

Le protocole HTTP (Hypertext Transfer Protocol) est un protocole d'application pour les systèmes d'information distribués, collaboratifs et hypermédia.

HTTP est la base de la communication de données pour le World Wide Web, où les documents hypertextes incluent des hyperliens vers d'autres ressources auxquelles l'utilisateur peut facilement accéder, par exemple par un clic de souris ou en tapant sur l'écran dans un navigateur Web.

HTTP a été développé pour faciliter l'hypertexte et le World Wide Web.

La fonction principale d'un serveur Web est de stocker, traiter et livrer des pages Web aux clients.

La communication entre le client et le serveur s'effectue à l'aide du protocole HTTP (Hypertext Transfer Protocol).

Les pages livrées sont le plus souvent des documents HTML, qui peuvent inclure des images, des feuilles de style et des scripts en plus du contenu textuel.

Plusieurs serveurs Web peuvent être utilisés pour un site Web à fort trafic.

Un agent d'utilisateur, généralement un navigateur Web ou un robot d'indexation Web, initie la communication en faisant une demande pour une ressource spécifique à l'aide de HTTP.

Le serveur répond par le contenu de cette ressource ou par un message d'erreur s'il est incapable de le faire. La ressource est généralement un fichier réel sur le stockage secondaire du serveur, mais ce n'est pas nécessairement le cas et dépend de la manière dont le serveur Web est implémenté.

## II. Consignes générales

• Votre programme ne doit en aucun cas crash (même si vous êtes à court de mémoire) ni s'arrêter de manière inattendue sauf dans le cas d'un comportement indéfini. Si cela arrive, votre projet sera considéré non fonctionnel et vous aurez 0.

• Vous devez rendre un Makefile qui compilera vos fichiers sources. Il ne doit pas relink.

• Votre Makefile doit contenir au minimum les règles suivantes : $(NAME), all, clean, fclean et re.

• Compilez votre code avec c++ et les flags -Wall -Wextra -Werror

• Vous devez vous conformer à la norme C++ 98. Par conséquent, votre code doit compiler si vous ajoutez le flag -std=c++98

• Dans votre travail, essayez d'utiliser en priorité des fonctionnalités C++ (par exemple, préférez <cstring> à <string.h>). Vous pouvez utiliser des fonctions C, mais faites votre possible pour choisir la version C++ quand vous le pouvez.

• Tout usage de bibliothèque externe ou de l'ensemble Boost est interdit.

## III. Partie obligatoire

| Élément | Description |
|---------|-------------|
| **Nom du programme** | webserv |
| **Fichiers de rendu** | Makefile, *.{h, hpp}, *.cpp, *.tpp, *.ipp, des fichiers de configuration |
| **Makefile** | Oui |
| **Arguments** | [Un fichier de configuration] |
| **Fonctions externes autorisées** | Tout ce qui respecte la norme C++ 98. execve, dup, dup2, pipe, strerror, gai_strerror, errno, dup, dup2, fork, socketpair htons, htonl, ntohs, ntohl, select, poll, epoll (epoll_create, epoll_ctl, epoll_wait), kqueue (kqueue, kevent), socket, accept, listen, send, recv, chdir bind, connect, getaddrinfo, freeaddrinfo, setsockopt, getsockname, getprotobyname, fcntl, close, read, write, waitpid, kill, signal, access, stat, open, opendir, readdir and closedir. |
| **Libft autorisée** | Non |
| **Description** | Un serveur HTTP en C++ 98 |

Vous devez écrire un serveur HTTP en C++ 98.
Votre binaire devra être appelé comme ceci :
```bash
./webserv [configuration file]
```

> **Note :** Bien que poll() soit mentionné dans le sujet et la grille d'évaluation, vous pouvez utiliser un équivalent tel que select(), kqueue(), ou epoll().

> **Important :** Veuillez lire la RFC et faire quelques tests avec telnet et NGINX avant de commencer ce projet. Même si vous n'avez pas à implémenter toute la RFC, cela vous aidera à développer les fonctionnalités requises.

### III.1 Prérequis

• Votre programme doit prendre un fichier de configuration en argument ou utiliser un chemin par défaut.

• Vous ne pouvez pas exécuter un autre serveur web.

• Votre serveur ne doit jamais bloquer et le client doit être correctement renvoyé si nécessaire.

• Il doit être non bloquant et n'utiliser qu'un seul poll() (ou équivalent) pour toutes les opérations entrées/sorties entre le client et le serveur (listen inclus).

• poll() (ou équivalent) doit vérifier la lecture et l'écriture en même temps.

• Vous ne devriez jamais faire une opération de lecture ou une opération d'écriture sans passer par poll() (ou équivalent).

• La vérification de la valeur de errno est strictement interdite après une opération de lecture ou d'écriture.

• Vous n'avez pas besoin d'utiliser poll() (ou équivalent) avant de lire votre fichier de configuration.

> **Attention :** Comme vous pouvez utiliser des FD en mode non bloquant, il est possible d'avoir un serveur non bloquant avec read/recv ou write/send tout en n'ayant pas recours à poll() (ou équivalent). Mais cela consommerait des ressources système inutilement. Ainsi, si vous essayez d'utiliser read/recv ou write/send avec n'importe quel FD sans utiliser poll() (ou équivalent), votre note sera de 0.

• Vous pouvez utiliser chaque macro et définir comme FD_SET, FD_CLR, FD_ISSET, FD_ZERO (comprendre ce qu'elles font et comment elles le font est très utile).

• Une requête à votre serveur ne devrait jamais se bloquer pour indéfiniment.

• Votre serveur doit être compatible avec le navigateur web de votre choix.

• Nous considérerons que NGINX est conforme à HTTP 1.1 et peut être utilisé pour comparer les en-têtes et les comportements de réponse.

• Vos codes d'état de réponse HTTP doivent être exacts.

• Votre serveur doit avoir des pages d'erreur par défaut si aucune n'est fournie.

• Vous ne pouvez pas utiliser fork pour autre chose que CGI (comme PHP ou Python, etc).

• Vous devriez pouvoir servir un site web entièrement statique.

• Le client devrait pouvoir téléverser des fichiers.

• Vous avez besoin au moins des méthodes GET, POST, et DELETE

• Stress testez votre serveur, il doit rester disponible à tout prix.

• Votre serveur doit pouvoir écouter sur plusieurs ports (cf. Fichier de configuration).

### III.2 Pour MacOS seulement

Vu que MacOS n'implémente pas write() comme les autres Unix, vous pouvez utiliser fcntl().

Vous devez utiliser des descripteurs de fichier en mode non bloquant afin d'obtenir un résultat similaire à celui des autres Unix.

Toutefois, vous ne pouvez utiliser fcntl() que de la façon suivante : F_SETFL, O_NONBLOCK et FD_CLOEXEC.

Tout autre flag est interdit.

### III.3 Fichier de configuration

Vous pouvez vous inspirer de la partie "serveur" du fichier de configuration NGINX.

Dans ce fichier de configuration, vous devez pouvoir :

• Choisir le port et l'host de chaque "serveur".

• Setup server_names ou pas.

• Le premier serveur pour un host:port sera le serveur par défaut pour cet host:port (ce qui signifie qu'il répondra à toutes les requêtes qui n'appartiennent pas à un autre serveur).

• Setup des pages d'erreur par défaut.

• Limiter la taille du body des clients.

• Setup des routes avec une ou plusieurs des règles/configurations suivantes (les routes n'utiliseront pas de regexp) :
  - Définir une liste de méthodes HTTP acceptées pour la route.
  - Définir une redirection HTTP.
  - Définir un répertoire ou un fichier à partir duquel le fichier doit être recherché (par exemple si l'url /kapouet est rootée sur /tmp/www, l'url /kapouet/pouic/toto/pouet est /tmp/www/pouic/toto/pouet).
  - Activer ou désactiver le listing des répertoires.
  - Set un fichier par défaut comme réponse si la requête est un répertoire.
  - Exécuter CGI en fonction de certaines extensions de fichier (par exemple .php).
  - Faites-le fonctionner avec les méthodes POST et GET.
  - Rendre la route capable d'accepter les fichiers téléversés et configurer où cela doit être enregistré.

> **Note CGI :** Souvenez-vous simplement que pour les requêtes fragmentées, votre serveur doit la dé-fragmenter et le CGI attendra EOF comme fin du body. Même choses pour la sortie du CGI. Si aucun content_length n'est renvoyé par le CGI, EOF signifiera la fin des données renvoyées.

Vous devez fournir des fichiers de configuration et des fichiers de base par défaut pour tester et démontrer que chaque fonctionnalité fonctionne pendant l'évaluation.

> **Important :** Si vous avez une question sur un comportement, vous devez comparer le comportement de votre programme avec celui de NGINX. Par exemple, vérifiez le fonctionnement du server_name.

> **Testing :** Nous avons partagé avec vous un petit testeur. Il n'est pas obligatoire de le réussir à la perfection si tout fonctionne bien avec votre navigateur et vos tests, mais cela peut vous aider à résoudre certains bugs. L'important, c'est la résilience. Votre serveur ne devrait jamais mourir.

> **Note :** Ne testez pas avec un seul programme. Écrivez vos tests avec un langage comme Python ou Golang, etc... Vous pouvez même les faire en C ou C++.

## IV. Compilation et Utilisation

### Compilation
```bash
make
```

### Lancement du serveur
```bash
# Avec fichier de configuration par défaut
./webserv

# Avec fichier de configuration spécifique
./webserv config.conf
```

### Nettoyage
```bash
make clean    # Supprime les fichiers objets
make fclean   # Supprime les fichiers objets et l'exécutable
make re       # Recompile entièrement
```

## V. Tests et Évaluation

### 🚀 Tests Rapides
```bash
# Test de base
curl -i http://localhost:8081/

# Test des locations spécifiques
curl -i http://localhost:8081/directory/
curl -i http://localhost:8081/directory/nop/
curl -i http://localhost:8081/directory/Yeah/

# Test autoindex
curl -i http://localhost:8081/YoupiBanane/

# Test des erreurs
curl -i http://localhost:8081/nonexistent

# Test CGI
curl -i http://localhost:8081/simple.cgi.py

# Test POST
curl -X POST -d "test=data" http://localhost:8081/

# Test DELETE
curl -X DELETE http://localhost:8081/test_file.txt

# Test avec plusieurs headers
curl -H "Accept: text/html" -H "User-Agent: curl/test" http://localhost:8081/
```

### 🧪 Tests Automatisés
```bash
# Test complet du serveur
python3 webserv_tester.py

# Test de stress (connexions multiples)
python3 LesTestsTaGeule/stress_test.py

# Test des timeouts
python3 LesTestsTaGeule/test_timeout.py

# Test des requêtes fragmentées
python3 LesTestsTaGeule/fragement_test.py

# Test spécifique YoupiBanane
python3 test_youpie_banana.py

# Test rapide des fonctionnalités
python3 test_fixes.py
```

### 📋 Checklist d'Évaluation Complète

#### ✅ **Fonctionnalités HTTP Obligatoires**
- [ ] **Serveur non-bloquant** avec epoll/kqueue/select
- [ ] **Méthodes HTTP** : GET, POST, DELETE minimum
- [ ] **Codes de statut HTTP** corrects (200, 404, 500, etc.)
- [ ] **En-têtes HTTP** conformes (Content-Type, Content-Length, etc.)
- [ ] **Support HTTP/1.1** complet
- [ ] **Gestion des timeouts** clients
- [ ] **Pages d'erreur** personnalisées

#### ✅ **Configuration Avancée**
- [ ] **Fichiers de configuration** type Nginx
- [ ] **Multiple server blocks** sur différents ports
- [ ] **Server names** et virtual hosts
- [ ] **Locations** avec règles spécifiques
- [ ] **Root et alias** pour les chemins
- [ ] **Index files** par défaut
- [ ] **Client max body size** configurable

#### ✅ **Fonctionnalités Dynamiques**
- [ ] **Upload de fichiers** multipart/form-data
- [ ] **Autoindex** (listing de répertoires)
- [ ] **CGI** (Python, PHP, etc.)
- [ ] **Redirections HTTP** (301, 302)
- [ ] **Méthodes par location** (restriction GET/POST/DELETE)

#### ✅ **Tests de Robustesse**
- [ ] **Requêtes malformées** → erreur 400
- [ ] **Gros fichiers** (>10MB) → respect max_body_size
- [ ] **Connexions simultanées** (50+ clients)
- [ ] **Stress test** prolongé (30+ minutes)
- [ ] **Memory leaks** → Valgrind clean
- [ ] **Crash test** → serveur ne meurt jamais
- [ ] **Requêtes fragmentées** → reconstruction correcte

#### ✅ **Performance et Scalabilité**
- [ ] **RPS** (Requests Per Second) > 10
- [ ] **Latence** < 100ms pour fichiers statiques
- [ ] **CPU usage** raisonnable sous charge
- [ ] **Memory usage** stable dans le temps

### 🔧 Guide de Débogage

#### 🚫 Le serveur ne démarre pas
```bash
# Vérifier le port
lsof -i :8081
netstat -tlnp | grep 8081

# Libérer le port si nécessaire
kill -9 $(lsof -t -i:8081)

# Vérifier les permissions
ls -la webserv
chmod +x webserv

# Vérifier la config
./webserv config.conf 2>&1 | head -20
```

#### ⚠️ Erreurs de compilation
```bash
# Vérifier GCC/G++
g++ --version
# Doit être >= 4.8 pour C++98

# Compilation propre
make fclean && make

# Debug avec flags
make CXXFLAGS="-Wall -Wextra -Werror -g -fsanitize=address"
```

#### 🐛 CGI ne fonctionne pas
```bash
# Permissions sur scripts
chmod +x www/tests/*.cgi.py
chmod +x www/main/tests/*.cgi.py

# Vérifier l'interpréteur
which python3
head -1 www/tests/simple.cgi.py

# Test manuel du script
python3 www/tests/simple.cgi.py

# Variables d'environnement CGI
export REQUEST_METHOD=GET
export QUERY_STRING=""
python3 www/tests/simple.cgi.py
```

#### 📊 Tests de performance
```bash
# Apache Bench (si installé)
ab -n 1000 -c 10 http://localhost:8081/

# Curl en boucle
for i in {1..100}; do curl -s http://localhost:8081/ > /dev/null; done

# Monitor ressources
top -p $(pgrep webserv)
```

### 🎯 Commandes d'Évaluation

#### Tests avec curl (basiques)
```bash
# GET simple
curl -v http://localhost:8081/

# POST avec données
curl -X POST -d "name=test&value=123" \
     -H "Content-Type: application/x-www-form-urlencoded" \
     http://localhost:8081/

# Upload de fichier
curl -X POST -F "file=@test_file.txt" http://localhost:8081/

# DELETE
curl -X DELETE http://localhost:8081/uploaded_file.txt

# Test avec headers custom
curl -H "Host: example.com" -H "User-Agent: Test/1.0" \
     http://localhost:8081/

# Test requête malformée
echo -e "GET / HTTP/1.1\r\nHost: invalid\r\n\r\n" | nc localhost 8081
```

#### Tests avec telnet (avancés)
```bash
# Connexion telnet
telnet localhost 8081

# Ensuite taper :
GET / HTTP/1.1
Host: localhost:8081

# Requête POST
POST / HTTP/1.1
Host: localhost:8081
Content-Length: 11

hello world

# Test keep-alive
GET / HTTP/1.1
Host: localhost:8081
Connection: keep-alive

```

### 📈 Métriques de Performance Attendues

| Métrique | Valeur Minimale | Valeur Excellente |
|----------|----------------|-------------------|
| **RPS** (Requests/sec) | 10+ | 100+ |
| **Latence moyenne** | < 100ms | < 10ms |
| **Connexions simultanées** | 50+ | 500+ |
| **Memory usage** | < 100MB | < 50MB |
| **CPU usage** | < 50% | < 20% |
| **Uptime sous stress** | 30min+ | 24h+ |

### 🏆 Bonus Points (Évaluation)

#### Fonctionnalités Bonus
- [ ] **Support HTTPS/SSL** 
- [ ] **Compression gzip**
- [ ] **Cache de fichiers statiques**
- [ ] **Load balancing**
- [ ] **Logs détaillés** avec rotation
- [ ] **Hot reload** de configuration
- [ ] **WebSocket support**
- [ ] **HTTP/2 support**

#### Qualité du Code
- [ ] **Documentation complète**
- [ ] **Tests unitaires**
- [ ] **Code coverage** > 80%
- [ ] **Architecture modulaire**
- [ ] **Gestion d'erreurs** robuste

---

## VI. Guide Technique

### 1. Qu'est-ce qu'un serveur HTTP ?

Un serveur HTTP est un programme qui:
- Écoute sur un port spécifique (comme 8080, 8081)
- Attend que des clients (navigateurs) lui envoient des requêtes
- Traite ces requêtes et renvoie des réponses (pages web, fichiers, etc.)

Imagine un restaurant:
- Le serveur = le serveur HTTP
- Les clients = les navigateurs web
- Les commandes = les requêtes HTTP
- Les plats servis = les réponses HTTP

### 2. Le fichier de configuration (.conf)

Un fichier .conf dit au serveur comment se comporter. C'est comme un manuel d'instructions.

server {
    listen 8081;                          # Écoute sur le port 8081
    server_name localhost 127.0.0.1;      # Noms du serveur
    root ./www/tests/;                    # Dossier racine des fichiers
    index index.html;                     # Fichier par défaut
    cgi_extension .py /usr/bin/python3;   # Scripts CGI Python
    error_page 404 www/errors/404.html;   # Page d'erreur 404
    upload_path ./www/tests/;             # Où sauver les fichiers uploadés
    client_max_body_size 10485760;        # Taille max (10MB)

    location /directory/ {                # Configuration pour /directory/
        root ./YoupiBanane;
        autoindex on;                     # Affiche la liste des fichiers
        allow_methods GET;                # Seule méthode autorisée
    }
    
    location / {                          # Configuration pour tout le reste
        root ./www/tests/;
        allow_methods GET POST DELETE;    # Méthodes autorisées
        upload_path ./www/tests/;
        client_max_body_size 10485760;
    }
}

3: Architecture du code

src/
├── main.cpp              # Point d'entrée
├── core/
│   ├── EpollClasse.cpp   # Gestion des événements I/O
│   └── TimeoutManager.cpp # Gestion des timeouts
├── config/
│   ├── Parser.cpp        # Lecture du fichier .conf
│   ├── Server.cpp        # Représentation d'un serveur
│   └── Location.cpp      # Représentation d'une location
├── http/
│   └── RequestBufferManager.cpp # Gestion des requêtes
└── cgi/
    └── CgiHandler.cpp    # Exécution de scripts

4: Fonctions cles explication

Le parser (config/Parser.cpp)
lit le fichier .conf et le trqnsforme en objects c++
void Parser::parseConfigFile(const std::string& configFile) {
    // 1. Ouvre le fichier
    std::ifstream file(configFile.c_str());
    
    // 2. Lit tout le contenu
    std::string content;
    std::string line;
    while (std::getline(file, line)) {
        content += line + " ";
    }
    
    // 3. Découpe en mots (tokens)
    tokenize(content);
    
    // 4. Analyse et crée les objets Server
    parseServers();
}

    - Lit le fichier ligne par ligne
    - ignore les commentaires (#)
    - Decoupe en mots-cles
    - Cree les objets Server avec leur Location

4.2: EpollClasse (core/EpollClasse.cpp)
Epoll est le coeur du serveur il surveille tous les descripteurs  de fichier (sockets):
void EpollClasse::serverRun() {
    while (true) {
        // Attend des événements sur les sockets
        int num_events = epoll_wait(_epoll_fd, _events, MAX_EVENTS, 1000);
        
        for (int i = 0; i < num_events; ++i) {
            int fd = _events[i].data.fd;
            
            if (isServerFd(fd)) {
                // Nouvelle connexion client
                acceptConnection(fd);
            } else if (_events[i].events & EPOLLIN) {
                // Données à lire du client
                handleRequest(fd);
            }
        }
    }
}

Pourquoi epoll ?
    - peut surveiller des milliers de connexions simultanement
    - Non-bloquant: ne reste pas bloque sur une seule connexion
    - Efficace: ne verifie que les sockets qui ont des donnees

4.3: geston des Requetes HTTP
Quand un client envoie une requete, le serveur:
void EpollClasse::handleRequest(int client_fd) {
    // 1. Lit les données du client
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);
    
    // 2. Accumule dans un buffer
    _bufferManager.append(client_fd, std::string(buffer, bytes_read));
    
    // 3. Vérifie si la requête est complète
    if (_bufferManager.isRequestComplete(client_fd)) {
        std::string request = _bufferManager.get(client_fd);
        
        // 4. Parse la requête
        std::string method = parseMethod(request);  // GET, POST, DELETE
        std::string path = parsePath(request);      // /index.html
        
        // 5. Traite selon la méthode
        if (method == "GET") {
            handleGetRequest(client_fd, path, server);
        } else if (method == "POST") {
            handlePostRequest(client_fd, path, body, headers, server);
        }
    }
}

5. Les methode HTTP
GET Recupere un fichier
void EpollClasse::handleGetRequest(int client_fd, const std::string &path, const Server &server) {
    // 1. Résout le chemin complet
    std::string fullPath = resolvePath(server, path);
    
    // 2. Vérifie si le fichier existe
    if (fileExists(fullPath)) {
        // 3. Lit le fichier
        std::string content = readFile(fullPath);
        
        // 4. Crée la réponse HTTP
        std::string response = generateHttpResponse(200, "text/html", content);
        
        // 5. Envoie au client
        sendResponse(client_fd, response);
    } else {
        // 6. Erreur 404
        sendErrorResponse(client_fd, 404, server);
    }
}

POST recevoir une donnees
void EpollClasse::handlePostRequest(int client_fd, const std::string &path, 
                                    const std::string &body, 
                                    const std::map<std::string, std::string> &headers, 
                                    const Server &server) {
    // 1. Vérifie la taille du body
    if (body.length() > server.client_max_body_size) {
        sendErrorResponse(client_fd, 413, server); // Payload Too Large
        return;
    }
    
    // 2. Détermine le type de contenu
    if (headers.find("Content-Type")->second.find("multipart/form-data") != std::string::npos) {
        // Upload de fichier
        handleFileUpload(client_fd, body, headers, server);
    } else {
        // Données simples - sauvegarde dans un fichier
        std::string fullPath = resolvePath(server, path);
        std::ofstream file(fullPath.c_str());
        file << body;
        file.close();
        
        std::string response = generateHttpResponse(201, "text/plain", "Created");
        sendResponse(client_fd, response);
    }
}

6: RequestBufferManager
Gere l'accumulation des donnees des requetes
bool RequestBufferManager::isRequestComplete(int client_fd) {
    const std::string& buffer = _buffers[client_fd];
    
    // 1. Vérifie si on a les en-têtes complets
    if (!hasCompleteHeaders(buffer)) {
        return false;
    }
    
    // 2. Récupère Content-Length
    size_t contentLength = getContentLength(buffer);
    
    // 3. Trouve où commencent les données
    size_t headerEnd = buffer.find("\r\n\r\n");
    size_t bodyStart = headerEnd + 4;
    
    // 4. Vérifie si on a tout le body
    size_t currentBodyLength = buffer.length() - bodyStart;
    return currentBodyLength >= contentLength;
}

7: autoindex
Genere une page listant les fichiers d'un dossier :
std::string AutoIndex::generateAutoIndexPage(const std::string &directoryPath) {
    DIR *dir = opendir(directoryPath.c_str());
    
    std::ostringstream html;
    html << "<html><body><h1>Index of " << directoryPath << "</h1><ul>";
    
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        std::string name = entry->d_name;
        if (name != "." && name != "..") {
            html << "<li><a href=\"" << name << "\">" << name << "</a></li>";
        }
    }
    
    closedir(dir);
    html << "</ul></body></html>";
    return html.str();
}

# Webserv - Guide Complet pour Débutants

Je vais t'expliquer en détail les concepts les plus importants de ce projet webserv, comme si tu découvrais tout pour la première fois.

## 1. Qu'est-ce qu'un serveur HTTP ?

Un **serveur HTTP** est un programme qui :
- Écoute sur un port spécifique (comme 8080, 8081)
- Attend que des clients (navigateurs) lui envoient des requêtes
- Traite ces requêtes et renvoie des réponses (pages web, fichiers, etc.)

Imagine un restaurant :
- Le serveur = le serveur HTTP
- Les clients = les navigateurs web
- Les commandes = les requêtes HTTP
- Les plats servis = les réponses HTTP

## 2. Le fichier de configuration (.conf)

### Qu'est-ce qu'un fichier .conf ?

Un fichier `.conf` (configuration) dit au serveur comment se comporter. C'est comme un manuel d'instructions.

Regardons ton fichier config.conf :

```properties
server {
    listen 8081;                    # Écoute sur le port 8081
    server_name localhost 127.0.0.1; # Noms du serveur
    root ./www/tests/;              # Dossier racine des fichiers
    index index.html;               # Fichier par défaut
    cgi_extension .py /usr/bin/python3; # Scripts CGI Python
    error_page 404 www/errors/404.html; # Page d'erreur 404
    upload_path ./www/tests/;       # Où sauver les fichiers uploadés
    client_max_body_size 10485760;  # Taille max (10MB)

    location /directory/ {          # Configuration pour /directory/
        root ./YoupiBanane;
        autoindex on;               # Affiche la liste des fichiers
        allow_methods GET;          # Seule méthode autorisée
    }
    
    location / {                    # Configuration pour tout le reste
        root ./www/tests/;
        allow_methods GET POST DELETE; # Méthodes autorisées
        upload_path ./www/tests/;
        client_max_body_size 10485760;
    }
}
```

### Explication ligne par ligne :

- **`listen 8081`** : Le serveur écoute sur le port 8081
- **`server_name`** : Noms acceptés (localhost, 127.0.0.1)
- **`root`** : Dossier où chercher les fichiers
- **`location`** : Règles spécifiques pour certains chemins

## 3. Architecture du Code

### Structure principale

```
src/
├── main.cpp              # Point d'entrée
├── core/
│   ├── EpollClasse.cpp   # Gestion des événements I/O
│   └── TimeoutManager.cpp # Gestion des timeouts
├── config/
│   ├── Parser.cpp        # Lecture du fichier .conf
│   ├── Server.cpp        # Représentation d'un serveur
│   └── Location.cpp      # Représentation d'une location
├── http/
│   └── RequestBufferManager.cpp # Gestion des requêtes
└── cgi/
    └── CgiHandler.cpp    # Exécution de scripts
```

## 4. Fonctions Clés Expliquées

### 4.1 Le Parser (config/Parser.cpp)

Le **Parser** lit le fichier `.conf` et le transforme en objets C++ :

```cpp
void Parser::parseConfigFile(const std::string& configFile) {
    // 1. Ouvre le fichier
    std::ifstream file(configFile.c_str());
    
    // 2. Lit tout le contenu
    std::string content;
    std::string line;
    while (std::getline(file, line)) {
        content += line + " ";
    }
    
    // 3. Découpe en mots (tokens)
    tokenize(content);
    
    // 4. Analyse et crée les objets Server
    parseServers();
}
```

**Que fait-il ?**
- Lit le fichier ligne par ligne
- Ignore les commentaires (#)
- Découpe en mots-clés
- Crée des objets `Server` avec leurs `Location`

### 4.2 EpollClasse (core/EpollClasse.cpp)

**Epoll** est le cœur du serveur. Il surveille tous les descripteurs de fichiers (sockets) :

```cpp
void EpollClasse::serverRun() {
    while (true) {
        // Attend des événements sur les sockets
        int num_events = epoll_wait(_epoll_fd, _events, MAX_EVENTS, 1000);
        
        for (int i = 0; i < num_events; ++i) {
            int fd = _events[i].data.fd;
            
            if (isServerFd(fd)) {
                // Nouvelle connexion client
                acceptConnection(fd);
            } else if (_events[i].events & EPOLLIN) {
                // Données à lire du client
                handleRequest(fd);
            }
        }
    }
}
```

**Pourquoi epoll ?**
- Peut surveiller des milliers de connexions simultanément
- Non-bloquant : ne reste pas bloqué sur une seule connexion
- Efficace : ne vérifie que les sockets qui ont des données

### 4.3 Gestion des Requêtes HTTP

Quand un client envoie une requête, le serveur :

```cpp
void EpollClasse::handleRequest(int client_fd) {
    // 1. Lit les données du client
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);
    
    // 2. Accumule dans un buffer
    _bufferManager.append(client_fd, std::string(buffer, bytes_read));
    
    // 3. Vérifie si la requête est complète
    if (_bufferManager.isRequestComplete(client_fd)) {
        std::string request = _bufferManager.get(client_fd);
        
        // 4. Parse la requête
        std::string method = parseMethod(request);  // GET, POST, DELETE
        std::string path = parsePath(request);      // /index.html
        
        // 5. Traite selon la méthode
        if (method == "GET") {
            handleGetRequest(client_fd, path, server);
        } else if (method == "POST") {
            handlePostRequest(client_fd, path, body, headers, server);
        }
    }
}
```

## 5. Les Méthodes HTTP

### GET - Récupérer un fichier

```cpp
void EpollClasse::handleGetRequest(int client_fd, const std::string &path, const Server &server) {
    // 1. Résout le chemin complet
    std::string fullPath = resolvePath(server, path);
    
    // 2. Vérifie si le fichier existe
    if (fileExists(fullPath)) {
        // 3. Lit le fichier
        std::string content = readFile(fullPath);
        
        // 4. Crée la réponse HTTP
        std::string response = generateHttpResponse(200, "text/html", content);
        
        // 5. Envoie au client
        sendResponse(client_fd, response);
    } else {
        // 6. Erreur 404
        sendErrorResponse(client_fd, 404, server);
    }
}
```

### POST - Recevoir des données

```cpp
void EpollClasse::handlePostRequest(int client_fd, const std::string &path, 
                                    const std::string &body, 
                                    const std::map<std::string, std::string> &headers, 
                                    const Server &server) {
    // 1. Vérifie la taille du body
    if (body.length() > server.client_max_body_size) {
        sendErrorResponse(client_fd, 413, server); // Payload Too Large
        return;
    }
    
    // 2. Détermine le type de contenu
    if (headers.find("Content-Type")->second.find("multipart/form-data") != std::string::npos) {
        // Upload de fichier
        handleFileUpload(client_fd, body, headers, server);
    } else {
        // Données simples - sauvegarde dans un fichier
        std::string fullPath = resolvePath(server, path);
        std::ofstream file(fullPath.c_str());
        file << body;
        file.close();
        
        std::string response = generateHttpResponse(201, "text/plain", "Created");
        sendResponse(client_fd, response);
    }
}
```

## 6. CGI (Common Gateway Interface)

### Qu'est-ce que CGI ?

CGI permet d'exécuter des scripts (Python, PHP, etc.) côté serveur :

```cpp
void EpollClasse::handleCgiRequest(int client_fd, const std::string &scriptPath, 
                                   const std::string &method, const std::string &queryString, 
                                   const std::string &body, 
                                   const std::map<std::string, std::string> &headers, 
                                   const Server &server) {
    // 1. Crée un processus fils
    CgiHandler cgiHandler(scriptPath, "/usr/bin/python3");
    
    // 2. Configure les variables d'environnement
    cgiHandler.setupCgiEnvironment(method, queryString, headers);
    
    // 3. Exécute le script
    CgiProcess* process = cgiHandler.executeCgi(body);
    
    // 4. Lit la sortie du script
    std::string output = cgiHandler.readCgiOutput(process);
    
    // 5. Envoie la réponse au client
    sendResponse(client_fd, output);
}
```

### Exemple de script CGI (`aaa.cgi.py`)

```python
#!/usr/bin/env python3
import os
import sys

# En-têtes CGI (pas de ligne de statut HTTP)
print("Content-Type: text/html")
print()  # Ligne vide obligatoire

# Récupère les variables d'environnement
method = os.environ.get('REQUEST_METHOD', 'GET')
query = os.environ.get('QUERY_STRING', '')

# Génère du HTML
html = f"""
<html>
<body>
    <h1>Script CGI</h1>
    <p>Méthode: {method}</p>
    <p>Query: {query}</p>
</body>
</html>
"""

print(html)
```

## 7. RequestBufferManager

Gère l'accumulation des données des requêtes :

```cpp
bool RequestBufferManager::isRequestComplete(int client_fd) {
    const std::string& buffer = _buffers[client_fd];
    
    // 1. Vérifie si on a les en-têtes complets
    if (!hasCompleteHeaders(buffer)) {
        return false;
    }
    
    // 2. Récupère Content-Length
    size_t contentLength = getContentLength(buffer);
    
    // 3. Trouve où commencent les données
    size_t headerEnd = buffer.find("\r\n\r\n");
    size_t bodyStart = headerEnd + 4;
    
    // 4. Vérifie si on a tout le body
    size_t currentBodyLength = buffer.length() - bodyStart;
    return currentBodyLength >= contentLength;
}
```

## 8. Autoindex

Génère une page listant les fichiers d'un dossier :

```cpp
std::string AutoIndex::generateAutoIndexPage(const std::string &directoryPath) {
    DIR *dir = opendir(directoryPath.c_str());
    
    std::ostringstream html;
    html << "<html><body><h1>Index of " << directoryPath << "</h1><ul>";
    
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        std::string name = entry->d_name;
        if (name != "." && name != "..") {
            html << "<li><a href=\"" << name << "\">" << name << "</a></li>";
        }
    }
    
    closedir(dir);
    html << "</ul></body></html>";
    return html.str();
}
```

## 9. Cycle de Vie d'une Requête

1. **Client se connecte** → `acceptConnection()` accepte la connexion
2. **Client envoie requête** → `handleRequest()` lit les données
3. **Requête complète** → Parse la méthode, le chemin, les en-têtes
4. **Traitement** :
   - Fichier statique → lit et envoie
   - Script CGI → exécute et envoie la sortie
   - Upload → sauvegarde le fichier
5. **Réponse** → Envoie la réponse HTTP au client
6. **Fermeture** → Ferme la connexion

## 10. Points Clés du Projet

### Non-bloquant
- Utilise `epoll` pour surveiller plusieurs connexions
- Jamais de `read`/`write` sans `epoll`
- Mode `O_NONBLOCK` sur tous les sockets

### Robuste
- Gestion des timeouts
- Vérification de la taille des requêtes
- Pages d'erreur personnalisées
- Nettoyage des ressources

### Conforme HTTP/1.1
- Codes de statut corrects (200, 404, 500, etc.)
- En-têtes appropriés
- Support des méthodes GET, POST, DELETE

Ce serveur est une version simplifiée mais fonctionnelle d'Apache ou Nginx ! 