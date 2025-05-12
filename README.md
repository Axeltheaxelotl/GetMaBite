<p align="center">
  <img src="https://i.pinimg.com/736x/ed/7f/b0/ed7fb01597ddfd722f0916835697de3a.jpg" alt="non" width=900" height="400">
</p>

<h1 align="center"> Webserv </h1>

Webserv est un projet de serveur HTTP développé en C++98, inspiré du fonctionnement de NGINX. Il permet de comprendre les bases du protocole HTTP et la gestion des connexions réseau à grande échelle.

## Qu'est-ce que `epoll` ?

`epoll` est une interface du noyau Linux permettant de gérer efficacement un grand nombre de connexions réseau simultanées. Contrairement à `select` ou `poll`, `epoll` est plus performant pour les serveurs modernes car il évite de parcourir toute la liste des descripteurs à chaque événement. Cela permet à Webserv de rester réactif même sous forte charge.

## Fonctionnalités principales

- Serveur HTTP non bloquant (utilisation de `epoll`)
- Support des méthodes HTTP : GET, POST, DELETE
- Gestion de plusieurs serveurs et ports (virtual hosting)
- Configuration avancée via un fichier de configuration inspiré de NGINX
- Gestion des routes, alias, index, autoindex (listing de répertoire)
- Limitation de la taille du body client
- Upload de fichiers
- Exécution de CGI (ex: Python)
- Pages d'erreur personnalisées
- Redirections HTTP

## Installation et compilation

```sh
make
```

`webserv`.

## Lancement :

```sh
./webserv [.conf]
```

Si aucun fichier n'est précisé, `config.conf` sera utilisé par défaut.

## Exemple :

```nginx
server {
    listen 8081;
    root ./www/tests/;
    index index.html;
    cgi_extension .py /usr/bin/python3;
}
```

### Obligatoire
- [x] Makefile conforme
- [x] Lecture et parsing du fichier de configuration
- [x] Gestion de plusieurs serveurs et ports
- [x] Méthodes HTTP : GET, POST, DELETE
- [x] Serveur non bloquant avec epoll
- [x] Gestion des routes, root, index, alias
- [x] Listing de répertoire (autoindex)
- [x] Pages d’erreur par défaut
- [x] Limitation de la taille du body client (verfier qu il refuses bien les bodies trop gros avec erreur 413)
- [x] Upload de fichiers
- [x] Suppression de fichiers (DELETE)
- [x] Gestion des redirections HTTP
- [ ] Gestion complète du CGI (fork, execve, pipes, variables d’environnement) simon si tu voit ca bouge toi le huk
- [x] Gestion correcte des fragments de requêtes HTTP (a verifier mais normalement ok RequestBufferManager)
- [ ] Gestion stricte C++98
- [x] Gestion des pages d’erreur personnalisées
- [ ] Support de plusieurs server_name par serveur (j'ai vraiment la flemme de faire ca)
- [x] Gestion stricte des allow_methods par location
- [ ] Stress tests et robustesse (a faire pour tester si il ne crash pas)
- [ ] Comparaison du comportement avec NGINX (pour comparer les headers, codes d etat, la gestion des erreurs, etc...)
- [ ] Gestion du timeout (pour le timeout sur les connexions)
- [ ] uploads multipart/form-data (POST) juste ecrit le body dans un fichier sans parser "Sans parser le multipart"

### Bonus
&emsp;- [inshalla g mal a la tete ez] Support cookies et gestion de session
&emsp;- [ ] Support de plusieurs CGI

## Contributeur

Le fils de pute de Simon

## Test

tests pour la Gestion des fragments de requêtes HTTP

**commande :**
  ```sh
  curl -v http://localhost:8081
  ```
  
  &emsp;. **But :** Tester une requete GET simple<br>
  &emsp;. Il envoie une requete HTTP complete (EN UN SEULE MORCEAU) puis affiche la reponse recue.

**Commande :**
```sh
curl -v -d "test=fragment" http://localhost:8081/
```

  &emsp;. **But :** Tester une requête POST avec un body.<br>
  &emsp;. Il envoie une requête HTTP POST avec le body "test=fragment" et l’en-tête Content-Length adapté.

**Commande :**
```sh
nc localhost 8081
```

&emsp;. **But :** Simuler une connexion TCP "manuelle" pour envoyer une requête HTTP en plusieurs fragments.
&emsp; tapes les commandes une par une exemple:
```sh
POST / HTTP/1.1
Host: localhost:8081
Content-Length: 11

test=fragment
```
&emsp; . verifier que le serveur essaie pas de traiter la requete tant qu'elle n'est pas complete<br>
&emsp; . que le serveur ne ferme pas ca connection prematurement<br>
&emsp; . et que la reponse HTTP et correcte meme si elle arrive ne plusieurs morceaux.


pas faire es test du haut c de la merde faire ca c plus mieux
```sh
import socket
s = socket.socket()
s.connect(('localhost', 8081))
s.send(b"POST / HTTP/1.1\r\nHost: localhost:8081\r\nContent-Length: 13\r\n\r\n")
s.send(b"test=fragment")
print(s.recv(4096).decode())
s.close()
```

---

<p align="left">
  <h3><b>- Descripteur de fichier</b></h3><br>
  Un descripteur de fichier (FD) c juste un nombre entier qui identifie une ressource ouverte par un programme :<br>
  un fichier, un socket reseau, etc...<br>
  Ex: quand tu ouvres un socket, le systeme te donne un FD (ex: 4)<br><br>

  <h3><b>- A quoi sert epoll</b></h3><br>
  epoll permet a un serveur de surveiller plein de descripteurs de fihciers en meme temps, pour savoir quand il y a<br>
  quelque chose a lire ou a ecrire dessus, sans gaspiller de ressources.<br><br>

  <h3><b>- J a vais le faire marcher mtn inschallah</b></h3><br>
  <b>1. Cree une instance epoll</b><br>
  
  ```sh
  int epoll_fd = epoll_create1(0);
  ```
  <b>2. Ajouter les sockets a surveiller</b><br>
  Pour chaque sockets (serveur ou client), tu l'ajoutes a epoll:<br>
  ```sh
  epoll_event envent;
  event.envents = EPOLLIN; // On veut savoir quand il y a des donnees a lire
  event.data.fd = socket_fd;
  epoll_ctl(epoll_fd, EPOLL_CTL_ADD, socket_fd, &event);
  ```
  <b>3. Attendre les evenements</b><br>
  ```sh
  int n = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
  ```
  quand un evenement arrive (ex: un client envoie des donnees), epoll te dit quel FD agir.<br>

  <b>4. Traiter l'evenement</b><br>
  - Si c'est un nouveau socket serveur: accepte une nouvelle connexion.<br>
  - Si c'est un client: lire ou ecrire les donnees.<br>

---


  <p align="center">
    <h3><b>- Schéma de fonctionnement du serveur epoll</b></h3>
  </p>

  ```plaintext
  +-------------------+
  |   Démarrage       |
  +-------------------+
           |
           v
  +-------------------+
  |  epoll_create1()  |
  +-------------------+
           |
           v
  +-----------------------------+
  |  Création sockets serveurs  |
  +-----------------------------+
           |
           v
  +-----------------------------+
  |  Ajout sockets serveurs à   |
  |        epoll (EPOLLIN)      |
  +-----------------------------+
           |
           v
  +-----------------------------+
  |      Boucle principale      |
  |      while (1)              |
  |   epoll_wait(...)           |
  +-----------------------------+
           |
           v
  +-----------------------------+
  | Pour chaque événement :     |
  +-----------------------------+
           |
           v
     +-------------------+      +-------------------+
     | FD = serveur ?    |----->|  accept()         |
     +-------------------+      +-------------------+
           |                           |
           | Non                       v
           v                   +-------------------+
  +-------------------+        | Ajout client FD   |
  | FD = client ?     |------->| à epoll (EPOLLIN) |
  +-------------------+        +-------------------+
           |
           v
  +-------------------+
  |  handleRequest()  |
  |  (lecture req)    |
  +-------------------+
           |
           v
  +-------------------+
  |  Générer réponse  |
  +-------------------+
           |
           v
  +-------------------+
  |  sendResponse()   |
  +-------------------+
           |
           v
  +-------------------+
  |  close(client_fd) |
  +-------------------+
  ```