
<p align="center">
<img src="https://i.pinimg.com/736x/2c/2c/eb/2c2cebe8f7d647efd430755693a28abf.jpg" alt="nique ta mere" width="800" height="300"/>
<img>
</p>


<h1 align="center">RESUMATION DU PROJET</h1>

1. **But général**

Ici c'est la classe qui gère le serveur HTTP avec epoll (le multiplexage, t'as capté) :
- Elle lance les sockets serveurs (tu peux écouter sur plusieurs ports/IPs si t'es chaud).
- Elle attend que des clients se pointent et envoie des requêtes.
- Elle lit/écrit sur les sockets selon ce que epoll lui dit.
- Elle gère les requêtes HTTP (GET, POST, DELETE) et balance les réponses.

2. **Explication des parties importantes**

- **Constructeur / Destructeur** :
  - Le constructeur crée le descripteur epoll (si ça fail, ça râle et ça quitte).
  - Le destructeur ferme le descripteur si besoin.

- **setupServers** :
  - Prend la liste des configs serveurs (ServerConfig) et la vraie config (Server).
  - Pour chaque serveur :
    - Ouvre le socket, le bind, le met en non-bloquant, etc.
    - Ajoute le FD du serveur à epoll pour surveiller les connexions.
    - Log les infos (root, index).

- **serverRun** :
  - Boucle infinie :
    - Attend les events avec epoll_wait.
    - Pour chaque event :
      - Si c'est de la lecture (EPOLLIN) :
        - Si c'est un FD serveur, accepte une nouvelle connexion.
        - Sinon, lit une requête client et la traite.
      - Si erreur ou fermeture, appelle handleError.

- **addToEpoll** :
  - Ajoute un FD à epoll pour surveiller les events.
  - Met à jour _biggest_fd si besoin (mais en vrai il sert à rien pour l'instant).

- **isServerFd** :
  - Check si un FD c'est un socket serveur (pour différencier client/serveur).

- **acceptConnection** :
  - Accepte une connexion entrante sur un socket serveur.
  - Met le FD client en non-bloquant.
  - Ajoute le FD client à epoll pour surveiller les lectures.
  - Log l'adresse du client.

- **resolvePath** :
  - Résout le chemin réel d'un fichier à partir de la config serveur et du chemin demandé.
  - Gère :
    - La racine / → renvoie le fichier index.
    - Les locations avec alias/root.
    - Sinon, concatène root + chemin demandé.

- **handleRequest** :
  - Lit la requête HTTP du client.
  - Parse la méthode, le chemin, le protocole.
  - Résout le chemin du fichier à servir.
  - Vérifie si le fichier existe :
    - Si oui, appelle la méthode adaptée (GET, POST, DELETE).
    - Sinon, renvoie une 404.
  - Ferme le FD client à la fin.

- **sendResponse** :
  - Envoie la réponse HTTP sur le FD client, en gérant les envois partiels.

- **handleGetRequest** :
  - Si le chemin est un dossier :
    - Si autoindex activé, génère une page d'index HTML.
    - Sinon, tente de servir le fichier index du dossier.
  - Si c'est un fichier, le lit et l'envoie.
  - Sinon, renvoie une erreur (403 ou 404).
  - Ferme le FD client à la fin.

- **handlePostRequest** :
  - Récupère le body de la requête.
  - Écrit le body dans le fichier cible.
  - Répond 201 Created ou 403 Forbidden.
  - Ferme le FD client à la fin.

- **handleDeleteRequest** :
  - Supprime le fichier demandé.
  - Répond 200 OK ou 404 Not Found.
  - Ferme le FD client à la fin.

- **handleError** :
  - Log une erreur sur un FD et ferme le FD.

- **setNonBlocking** :
  - Met un FD en mode non-bloquant.

3. **Fonctions ou trucs pas utilisés/à revoir**

- **resolvePath** :
  - Elle est bien codée mais pas utilisée dans handleRequest !
  - Tu pourrais l'utiliser pour gérer les alias, locations, etc. comme prévu.

- **_biggest_fd** :
  - Il est mis à jour mais jamais utilisé (optimisation possible mais pas faite, tu peux le virer si tu veux).

- **Gestion multi-serveur** :
  - Pour l'instant tu utilises toujours _serverConfigs[0] (le premier serveur). Si tu veux gérer plusieurs serveurs virtuels (host/port différents), faudra améliorer ça.

4. **Conclusion**

Le code est bien structuré, il fait le taf pour un serveur HTTP multiplexé avec epoll. Le seul truc vraiment pas utilisé c'est resolvePath : tu devrais l'appeler dans handleRequest pour que la gestion des chemins soit propre (locations, alias, etc.).

Si tu veux des exemples de correction ou d'amélioration, hésite pas à demander !

---

<h3 align="center">
Brrr, skibidi bop bop dob yes yes<br>
Skibidi W nim nim<br>
Skibidi bop bop dob yes yes<br>
Skibidi W nim nim
</h2>

<p align="center">
<img src="https://i.pinimg.com/736x/8f/72/48/8f7248f31c0208a33947f3fa501301e4.jpg" alt="nique ta mere" width="300" height="400"/>
<img>
</p>