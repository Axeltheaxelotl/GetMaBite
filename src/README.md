## Les fonctions les plus importantes :

#### 1. EpollClasse::handleRequest(int client_fd)

##### Rôle :

- Elle lit la requête HTTP du client, la parse, choisit l'action, gère les erreurs, et envoie la réponse.

##### Ce qu’elle fait :

1. Lecture de la requête sur le socket client.
2. Gestion des buffers (pour les requêtes fragmentées).
3. Parsing.
4. Recherche de la bonne location (route) et des méthodes autorisées.
5. Résolution du chemin.
6. Vérification de l’existence du fichier ou dossier demandé.
7. Appel de la bonne fonction selon la méthode.
8. Gestion des erreurs.
9. Fermeture du socket à la fin.

#### 2. EpollClasse::sendErrorReponse(int client_fd, int code, const Server& server, const std::string& allowHeader="")

##### Rôle :

- Envoie une page d’erreur HTTP personnalisée ou par défaut au client.

##### Ce qu’elle fait :

1. Cherche si une page d’erreur personnalisée existe pour le code (dans server.error_pages).
2. Si oui, lit le fichier et l’envoie.
3. Sinon, génère une page d’erreur par défaut.
4. Ajoute le header Allow si c’est une erreur 405.
5. Envoie la réponse HTTP complète au client.

Permet de centraliser toute la gestion des erreurs HTTP. Supporte les pages d’erreur custom (dans error), le header 405 est obligatoire pour 405.

#### 3. RequestBufferManager::isRequestComplete(int client_fd, const Server& server)

##### Rôle :

- Détermine si la requête HTTP reçue est complète (headers + body).

##### Ce qu’elle fait :

1. Cherche la fin des headers (`\r\n\r\n`).
2. Si la méthode attend un body (POST/PUT), vérifie la présence du body complet selon Content-Length.
3. Vérifie que la taille du body ne dépasse pas client_max_body_size.
4. Retourne true si la requête est complète, sinon false.

Essentiel pour gérer les requêtes fragmentées (TCP n’est pas "packetisé"), permet de refuser les body trop gros (413).

#### 4. EpollClasse::resolvePath(const Server &server, const std::string &requestedPath)

##### Rôle :

- Calcule le chemin réel sur le disque à partir de l’URL demandée.

##### Ce qu’elle fait :

1. Si `/`, retourne le fichier index du serveur.
2. Cherche la location la plus longue qui matche le chemin demandé.
3. Applique le root ou l’alias de la location.
4. Retourne le chemin absolu à ouvrir/lire.

Gère la logique des roots, root, alias, index. Crucial pour la sécurité (évite les accès hors du dossier root).

#### 5. CookieManager::createSetCookieHeader(...)

##### Rôle :

- Génère l’en-tête Set-Cookie pour la réponse HTTP.

##### Ce qu’elle fait :

1. Prend le nom, la valeur, et les options du cookie.
2. Construit la chaîne `Set-Cookie: ...` avec les bons attributs (Path, Max-Age, HttpOnly, Secure, SameSite).
3. Retourne la chaîne prête à être ajoutée à la réponse.

Permet de gérer la sécurité des cookies (HttpOnly, Secure, SameSite). Utilisé pour le compteur de visites, sessions, etc.

#### 6. AutoIndex::generateAutoIndexPage(const std::string &directoryPath)

##### Rôle :

- Génère dynamiquement une page HTML listant les fichiers d’un dossier.

##### Ce qu’elle fait :

1. Ouvre le dossier demandé.
2. Liste tous les fichiers et dossiers (sauf `.` et `..`).
3. Génère une page HTML avec des liens vers chaque fichier.

Utilisé quand `autoindex on` est activé et qu’aucun index n’est trouvé. Permet de naviguer dans les dossiers via le navigateur.

#### 7. ServerRouter::findServerIndex(...)

##### Rôle :

- Trouve le bon bloc server à utiliser selon le host et le port de la requête.

##### Ce qu’elle fait :

1. Parcourt tous les serveurs configurés.
2. Cherche un serveur qui écoute sur le bon port et dont le `server_name` matche le host.
3. Retourne l’index du serveur trouvé, ou celui par défaut pour le port.

Permet le virtual hosting (plusieurs sites sur le même port). Utilise le header Host de la requête HTTP.
