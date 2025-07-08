              - Supprimer la déclaration dupliquée de `ErreurDansTaGrosseDaronne` dans **Utils.hpp**.  
              - Enlever ou compléter le fichier vide **Logger.cpp** (le logger est déjà header-only).  
              - Uniformiser les `#include` C vs C++ (`<cstring>` plutôt que `<string.h>`, etc.).  
              - Corriger le commentaire du constructeur `TimeoutManager(10)` (timeout en secondes) si l’intention était 60 s.

              ## 2. Nettoyage & robustesse  
              - Gérer proprement le cas où `Parser` rencontre une directive inconnue sans `exit()`.  
              - Centraliser la gestion des erreurs et éviter les `exit(1)` dispersés (préférer lever exception ou retourner un code d’erreur).  
              - Remplacer tous les `printf`/`vprintf` par des `std::cout` + `vfprintf(stderr,…)` pour ne pas mixer iostream et stdio.

## 3. Fonctionnalités manquantes ou incomplètes  
              - **Parsing de l’en-tête `Host`** et appel à `findMatchingServer(host, port)` au lieu de `serverConfigs[0]`.  
- Détection et exécution de CGI :  
  - Dans `EpollClasse::handleRequest`, tester l’extension contre `server.cgi_extensions` ou `location.cgi_extensions`.  
  - Appeler `handleCGI(...)` plutôt que stat/serve statique.  
- Implémenter la méthode **HEAD** (identique à GET sans body).  
- Respecter `Location::return_code`/`return_url` (directive `return`) avant tout autre traitement.  
- Traiter la directive `upload_path` (POST → enregistrer sous ce chemin).

## 4. Boucle epoll & écriture non bloquante  
- Compléter `serverRun()` :  
  - distinguer événements `EPOLLIN`/`EPOLLOUT`, appeler `handleRequest` ou envoyer la réponse en deux temps.  
  - gérer l’envoi partiel (sur plusieurs EPOLLOUT).  
- Supprimer les `…` dans `isServerFd`, `findMatchingServer`, `acceptConnection`, etc., et y ajouter la logique complète.

## 5. Tests & conformité HTTP  
- Ajouter des pages d’erreur par défaut si aucune `error_page` n’est configurée.  
- Vérifier la conformité des codes de statut et les headers minimaux (`Connection`, `Content‐Type`, `Content‐Length`).  
- Faire des tests automatisés (curl, Python, navigateur) pour GET, POST (upload), DELETE, CGI, redirections, auto-indexing.

               ## Points d’implémentation à revoir  
              - Vous acceptez trop vite `serverConfigs[0]` : il faut router selon l’en-tête `Host`.  
- Le parsing CGI (récupération des paramètres via `rfind("?")` sur le chemin système) est incorrect : il faut extraire la query string avant de résoudre le chemin de fichier.  
- `TimeoutManager` est initialisé à 10 s alors que le commentaire indique 60 s.  
- Dans Utils.hpp, vous déclarez deux fois la même fonction.  
- `Logger` utilise à la fois `std::cout` et `vprintf` : risqué.  
- `Location::alias` et `root` peuvent entrer en conflit : votre parser interdit les deux mais EpollClasse ne gère pas toujours correctement l’`alias`.  
- `EpollClasse::handleGetRequest` et `handleDeleteRequest` ferment systématiquement le client sans gérer la réinscription EPOLLOUT pour un envoi progressif.  
- Pas de prise en charge de HEAD, ni de chunked-encoding, ni de Keep-Alive.  
- La suppression du fichier statique en POST (upload) n’est pas conforme au sujet : il faut écrire dans le répertoire `upload_path`.