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

## Explication de non bloquant pour les gogols

le mode non bloquant c se qui permet a un programme de ne jamais rester "bloquee mdr" en attende de lors d'une operation d'entree/sortie (I/O) comme read(),<br>
write(), accept().<br>
Au lieu d'attendre qu'une donnee arrive (ce qui peut prendre du "temps"), la fonction retourne tout de suite:<br>
&emsp; . Si l'operation  peut etre faite, elle reussit normalement.<br>
&emsp; . Si ce n'est pas possible (ex: bas rien), elle retourne une erreur speciale (EGAIN ou EWOULDBLOCK).<br>

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
- [x] Limitation de la taille du body client (vérifier qu’il refuse bien les bodies trop gros avec erreur 413)
- [x] Upload de fichiers
- [x] Suppression de fichiers (DELETE)
- [x] Gestion des redirections HTTP
- [🖕] Gestion complète du CGI (fork, execve, pipes, variables d’environnement)
- [x] Gestion correcte des fragments de requêtes HTTP (à vérifier mais normalement ok RequestBufferManager)
- [ ] Gestion stricte C++98
- [x] Gestion des pages d’erreur personnalisées
- [x] Support complet des server_name par serveur (à vérifier avec des tests)
- [x] Gestion stricte des allow_methods par location
- [x] Stress tests et robustesse (tester si le serveur ne crash pas sous forte charge ou avec des requêtes malformées)
- [ ] Comparaison avec NGINX (vérifier les headers, codes d’état, gestion des erreurs, etc.)
- [🖕] Gestion du timeout (VRAIMENT BOFBOF A REVOIR)
- [ ] uploads multipart/form-data (POST) juste écrire le body dans un fichier sans parser "Sans parser le multipart"

### Non explicitement demandés mais fortement recommandés

- [ ] Gestion stricte des headers HTTP (conformité aux standards HTTP, RFC 7230)
- [ ] Support des cookies et gestion de session (bonus)

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

  <p>
  <h3>RESUME:</h3>
  &emsp; - Les sockets serveurs sont surveilles par epoll.<br>
  &emsp; - Quand un client se connecte, accept() et ajout un epoll.<br>
  &emsp; - Quand un client envoie une requete, handleRequest lit, traite, repond, puis ferme<br>
  &emsp; - Tout passe par epoll, aucune operation bloquante.<br>

  ---

  <h2>Les derniers points manquants ou a ameliorer</h2>
  <p>
  <b>1. Support complet des server_name:</b><br>
    &emsp; *Implemente la gestion de plusieurs server_name par server
  </p>
  <p>
  <b>2. Gestion stricte des erreurs HTTP:</b><br>
  &emsp; *Verifier que tous les codes d'etat HTTP sont exacts et que les pages d'erreur<br>
  &emsp; personnalisees sont servies correctement.
  </p>
  <p>
  <b>5.Timeouts:</b><br>
  &emsp; *Implementation d une gestion de timeouts pour les connexions inactives.


---


  <h2>Points intégrés et améliorations possibles</h2>
  <p>
  <b>Ce qui est déjà intégré :</b><br>
  1. <u>Gestion des méthodes HTTP</u> :<br>
    &emsp; - Méthodes supportées : `GET`, `POST`, `DELETE`.<br>

  2. <u>Gestion des routes et des fichiers</u> :<br>
    &emsp; - Gestion des routes, `root`, `index`, et `alias`.<br>
    &emsp; - Listing de répertoire (`autoindex`).<br>
    &emsp; - Upload et suppression de fichiers.<br>

  3. <u>Pages d'erreur personnalisées</u> :<br>
    &emsp; - Pages d'erreur pour les codes HTTP comme 400, 401, 403, 404, 500.<br>

  4. <u>Limitation de la taille du body client</u> :<br>
    &emsp; - Vérification et rejet des bodies trop gros avec une erreur 413.<br>

  5. <u>Gestion des fragments de requêtes HTTP</u> :<br>
    &emsp; - Utilisation de `RequestBufferManager` pour gérer les requêtes fragmentées.<br>

  6. <u>Redirections HTTP</u> :<br>
    &emsp; - Gestion des redirections via des codes comme 301 ou 302.<br>

  7. <u>Support des `server_name`</u> :<br>
    &emsp; - Gestion de plusieurs `server_name` par serveur.<br>

  8. <u>Stress tests et robustesse</u> :<br>
    &emsp; - Tests pour vérifier la stabilité sous forte charge ou avec des requêtes malformées.<br>
  </p>
  <p>
  <b>Ce que vous pourriez ajouter :</b><br>
  1. <u>Gestion stricte des headers HTTP</u> :<br>
    &emsp; - Vérification de la conformité des headers avec la RFC 7230.<br>
    &emsp; - Validation des headers obligatoires comme `Host` et gestion des headers optionnels.<br>

  2. <u>Support des cookies et gestion de session</u> :<br>
    &emsp; - Implémentation d'un gestionnaire de cookies pour lire, écrire et supprimer des cookies.<br>

  3. <u>Gestion stricte des timeouts</u> :<br>
    &emsp; - Déconnexion des connexions inactives après un certain délai.<br>

  4. <u>Support des requêtes multipart/form-data</u> :<br>
    &emsp; - Parsing des requêtes multipart pour gérer les uploads de fichiers complexes.<br>

  5. <u>Comparaison avec NGINX</u> :<br>
    &emsp; - Vérification des headers, des codes d'état, et des comportements pour s'assurer de la conformité.<br>

  6. <u>Gestion des connexions persistantes</u> :<br>
    &emsp; - Implémentation de `Connection: keep-alive` pour réutiliser les connexions.<br>

  7. <u>Support des encodages</u> :<br>
    &emsp; - Gestion des encodages comme `gzip` ou `chunked`.<br>

  8. <u>Gestion des méthodes HTTP supplémentaires</u> :<br>
    &emsp; - Ajout de méthodes comme `PUT`, `HEAD`, ou `OPTIONS`.<br>

  9. <u>Amélioration des logs</u> :<br>
    &emsp; - Ajout de logs détaillés pour les requêtes et réponses HTTP.<br>

  10. <u>Gestion des erreurs HTTP supplémentaires</u> :<br>
    &emsp; - Implémentation de codes d'erreur comme 405 (Method Not Allowed) ou 501 (Not Implemented).<br>
  </p>

### Qu'est-ce que la RFC ?

La RFC (Request for Comments) est une série de documents techniques et organisationnels publiés par l'IETF (Internet Engineering Task Force). Ces documents définissent les standards et protocoles utilisés sur Internet, comme HTTP, TCP/IP, ou encore DNS. Chaque RFC est numérotée et sert de référence pour les développeurs et ingénieurs.

#### À quoi sert la RFC dans Webserv ?
Dans le cadre de Webserv, la RFC 7230 (et les autres RFC liées à HTTP/1.1) est utilisée pour garantir que le serveur respecte les standards du protocole HTTP. Cela permet :
- Une compatibilité avec les navigateurs et clients HTTP.
- Une gestion correcte des requêtes et réponses HTTP.
- Une conformité aux bonnes pratiques pour éviter des comportements imprévus.









• Le programme doit prendre un fichier de configuration en argument ou utiliser un chemin par default.
• Vous ne pouvez pas exécuter un autre serveur web
• Votre serveur ne doit jamais bloquer et le client doit être correctement renvoyé si
nécessaire
• Il doit être non bloquant et n’utiliser qu’un seul epoll() pour
toutes les opérations entrées/sorties entre le client et le serveur
• epoll() doit verifier la lecture et l ecriture en meme temps
• Vous ne devriez jamais faire une opération de lecture ou une opération d’écriture sans passer par epoll()
• La vérification de la valeur de errno est strictement interdite après une opération de lecture ou d’écriture.
• Vous n’avez pas besoin d’utiliser epoll() (ou équivalent) avant de lire votre fichier de configuration.
• Vous pouvez utiliser chaque macro et définir comme FD_SET, FD_CLR, FD_ISSET,
FD_ZERO (comprendre ce qu’elles font et comment elles le font est très utile).
• Une requête à votre serveur ne devrait jamais se bloquer pour indéfiniment.
• Votre serveur doit être compatible avec le navigateur web de votre choix.
• Nous considérerons que NGINX est conforme à HTTP 1.1 et peut être utilisé pour
comparer les en-têtes et les comportements de réponse.
• Vos codes d’état de réponse HTTP doivent être exacts.
• Votre serveur doit avoir des pages d’erreur par défaut si aucune n’est fournie.
• Vous ne pouvez pas utiliser fork pour autre chose que CGI (comme PHP ou Python,
etc).
• Vous devriez pouvoir servir un site web entièrement statique.
• Le client devrait pouvoir téléverser des fichiers.
• Vous avez besoin au moins des méthodes GET, POST, et DELETE
• Stress testez votre serveur, il doit rester disponible à tout prix.
• Votre serveur doit pouvoir écouter sur plusieurs ports (cf. Fichier de configuration).

Vous pouvez vous inspirer de la partie "serveur" du fichier de
configuration NGINX.
Dans ce fichier de configuration, vous devez pouvoir :
• Choisir le port et l’host de chaque "serveur".
• Setup server_names ou pas.
• Le premier serveur pour un host:port sera le serveur par défaut pour cet host:port
(ce qui signifie qu’il répondra à toutes les requêtes qui n’appartiennent pas à un
autre serveur).
• Setup des pages d’erreur par défaut.
• Limiter la taille du body des clients.
• Setup des routes avec une ou plusieurs des règles/configurations suivantes (les
routes n’utiliseront pas de regexp) :
◦ Définir une liste de méthodes HTTP acceptées pour la route.
◦ Définir une redirection HTTP.
◦ Définir un répertoire ou un fichier à partir duquel le fichier doit être recherché
(par exemple si l’url /kapouet est rootée sur /tmp/www, l’url /kapouet/pouic/toto/pouet
est /tmp/www/pouic/toto/pouet).
◦ Activer ou désactiver le listing des répertoires.
7
Webserv C’est le moment de comprendre pourquoi les URLs commencent par HTTP !
◦ Set un fichier par défaut comme réponse si la requête est un répertoire.
◦ Exécuter CGI en fonction de certaines extensions de fichier (par exemple .php).
◦ Faites-le fonctionner avec les méthodes POST et GET.
◦ Rendre la route capable d’accepter les fichiers téléversés et configurer où cela
doit être enregistré.
— Vous vous demandez ce qu’est un CGI ?
— Parce que vous n’allez pas appeler le CGI mais utiliser directement le chemin
complet comme PATH_INFO.
— Souvenez-vous simplement que pour les requêtes fragmentées, votre serveur
doit la dé-fragmenter et le CGI attendra EOF comme fin du body.
— Même choses pour la sortie du CGI. Si aucun content_length n’est renvoyé
par le CGI, EOF signifiera la fin des données renvoyées.
— Votre programme doit appeler le CGI avec le fichier demandé comme premier argument.
— Le CGI doit être exécuté dans le bon répertoire pour l’accès au fichier de
chemin relatif.
— votre serveur devrait fonctionner avec un seul CGI (php-CGI, Python, etc.).
Vous devez fournir des fichiers de configuration et des fichiers de base par défaut pour
tester et démontrer que chaque fonctionnalité fonctionne pendant l’évaluation.





















• Beaucoup de directives non traitées (server_name, root/alias, methods, error_page, cgi_extensions…)
• Pas de client_max_body_size, pages d’erreur par défaut, redirections, autoindex, upload_path

          epoll & non-blocage
          • Seulement EPOLLIN, jamais EPOLLOUT
          • read()/write() hors epoll (bloquant)
          • Vérification d’errno après I/O (interdit)

CGI
• setCgiHeaders() inutilisé
• Corps POST mis en ENV au lieu du stdin
• Pas de vars CGI standard (CONTENT_TYPE, PATH_INFO…)
• Pas de chdir() avant execve
• Lecture bloquante des pipes CGI

Upload & multipart
• Pas de parsing multipart/form-data
• handlePostRequest() vide, pas d’écriture disque

          GET/HEAD/DELETE
          • Pas de HEAD spécifique
          • Implémentations non vérifiées

          Timeout & nettoyage
          • TimeoutManager non intégré à la boucle epoll
          • FDs non fermés sur erreur

Routing & virtual hosts
• Deux handlers redondants (ServerNameHandler vs ServerRouter)
• Pas de serveur par défaut si host inconnu

Pages d’erreur
• error_pages jamais lues ni servies
• Utils::ErreurDansTaGrosseDaronne non utilisé

Robustesse & sécurité
• Fuites de FDs en cas d’erreur
• substr/find_last_of sans test de npos

Tests & HTTP1.1
• Peu ou pas de tests unitaires/intégration
• En-têtes (keep-alive, chunked…) pas validés