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



SIMON<<>>
- Détection et exécution de CGI :  
  - Dans `EpollClasse::handleRequest`, tester l’extension contre `server.cgi_extensions` ou `location.cgi_extensions`.  
  - Appeler `handleCGI(...)` plutôt que stat/serve statique.
SIMON<<>>



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


















Voici une liste des éléments à tester et des fonctionnalités qui ne sont pas encore implémentées ou mal faites dans votre projet :

### **Fonctionnalités à tester :**
1. **Directives de configuration :**
   - Vérifier que toutes les directives dans les fichiers `.conf` sont correctement interprétées (exemple : `limit_except`, `client_max_body_size`).
   - Tester les directives `error_page`, `autoindex`, `root`, et `index`.

2. **Gestion des méthodes HTTP :**
   - Tester les méthodes HTTP prises en charge (`GET`, `POST`, `PUT`, `DELETE`, `HEAD`).
   - Vérifier les restrictions de méthode (`limit_except`).

3. **Gestion des erreurs HTTP :**
   - Tester les pages d'erreur personnalisées (`error_page`).
   - Vérifier les codes de statut HTTP (exemple : 404, 403, 500).

4. **Auto-indexing :**
   - Tester la génération de pages d'auto-index pour les répertoires.

5. **Redirections HTTP :**
   - Tester les redirections (`return` directive).

6. **Gestion des fichiers :**
   - Tester l'upload de fichiers via `POST`.
   - Vérifier la suppression de fichiers via `DELETE`.

7. **Server Name Matching :**
   - Tester la correspondance des noms de serveur (`server_name`).

8. **Multi-port et multi-serveur :**
   - Tester la gestion de plusieurs ports et serveurs.

9. **Timeouts :**
   - Vérifier la gestion des timeouts pour les connexions.

10. **Conformité HTTP :**
    - Vérifier les en-têtes HTTP minimaux (`Content-Type`, `Content-Length`, etc.).
    - Tester les requêtes chunked.

### **Fonctionnalités non implémentées ou mal faites :**
            1. **Directive `limit_except` :**
               - Non reconnue par le parser. Vous devez l'implémenter ou la retirer.

            2. **Directive `client_max_body_size` :**
               - Mal placée dans les blocs `location`. Elle doit être dans les blocs `server`.

3. **Gestion des CGI :**
   - La gestion des CGI est partiellement implémentée mais nécessite des ajustements (exemple : parsing des paramètres CGI).

      4. **Directive `upload_path` :**
         - Non prise en charge pour les uploads via `POST`.

5. **Routing basé sur l'en-tête `Host` :**
   - Le routage ne semble pas utiliser correctement l'en-tête `Host`.

6. **Boucle epoll :**
   - La gestion des événements `EPOLLIN` et `EPOLLOUT` est incomplète.
   - L'envoi partiel des réponses n'est pas géré.

7. **Gestion des erreurs dans le parser :**
   - Les erreurs de parsing ne sont pas centralisées et utilisent `exit()` au lieu de lever des exceptions.

8. **Tests automatisés :**
   - Les tests pour les fonctionnalités comme les redirections, les uploads, et les timeouts sont absents ou incomplets.

9. **Pages d'erreur par défaut :**
   - Si aucune page d'erreur n'est configurée, le serveur ne semble pas fournir de page par défaut.

10. **Conformité HTTP :**
    - Les codes de statut et les en-têtes HTTP ne sont pas toujours conformes.

### **Prochaines étapes :**
- Corriger les directives mal placées ou non reconnues.
- Implémenter les fonctionnalités manquantes (exemple : `upload_path`, gestion des CGI).
- Compléter la boucle epoll pour gérer les événements correctement.
- Ajouter des tests automatisés pour valider les fonctionnalités.