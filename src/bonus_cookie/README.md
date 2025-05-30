# Gestion des Cookies dans le Programme

## Qu'est-ce qu'un cookie ?
Un cookie est une petite donnée envoyée par le serveur au navigateur et stockée localement. Dans ce programme, il est utilisé pour suivre le nombre de visites d'un utilisateur sur la page principale.

## Fonctionnement des cookies
1. **Première requête** :
   - Le serveur vérifie si le cookie `visit_count` existe.
   - Si non, il est initialisé à `0`.
   - Le serveur incrémente la valeur et renvoie un cookie avec `Set-Cookie`.

2. **Requêtes suivantes** :
   - Le navigateur renvoie automatiquement le cookie.
   - Le serveur lit, incrémente et met à jour la valeur.

## Routes principales
- **`/`** :
  - Gère le cookie `visit_count`.
  - Affiche le nombre de visites.

- **`/tests/post_result.txt`** :
  - **POST** : Écrit des données dans le fichier.
  - **DELETE** : Supprime le fichier.
  - **PUT** : Recrée le fichier avec un contenu par défaut.

## Explication technique des cookies

Les cookies sont des éléments essentiels dans le fonctionnement des serveurs HTTP. Voici une explication technique détaillée de leur fonctionnement, adaptée à votre implémentation.

### 1. Qu'est-ce qu'un cookie techniquement ?
Un cookie est une paire clé-valeur envoyée par un serveur HTTP au client (navigateur) via l'en-tête `Set-Cookie`. Le navigateur stocke ce cookie et le renvoie automatiquement au serveur dans l'en-tête `Cookie` lors des requêtes suivantes.

### 2. Fonctionnement des cookies dans votre programme

#### Création d'un cookie
- **En-tête `Set-Cookie`** :
  - Lorsqu'une réponse HTTP est envoyée, le serveur peut inclure un en-tête `Set-Cookie` pour demander au navigateur de stocker un cookie.
  - Ce code génère un en-tête `Set-Cookie` avec des options comme :
    - **`Max-Age`** : Durée de vie du cookie en secondes.
    - **`Path`** : Chemin où le cookie est valide.
    - **`HttpOnly`** : Empêche l'accès au cookie via JavaScript.
    - **`Secure`** : Le cookie est envoyé uniquement via HTTPS.
    - **`SameSite`** : Limite l'envoi du cookie aux requêtes provenant du même site.

#### Stockage côté client
- Une fois reçu, le navigateur stocke le cookie localement avec ses attributs (nom, valeur, domaine, chemin, etc.).

#### Renvoi au serveur
- Lors de chaque requête vers le domaine et le chemin spécifiés, le navigateur inclut le cookie dans l'en-tête `Cookie`.

#### Lecture des cookies côté serveur
- Le serveur lit l'en-tête `Cookie` pour extraire les cookies envoyés par le client.

### 3. Fonctionnement des cookies dans votre serveur

Webserv C’est le moment de comprendre pourquoi les URLs commencent par HTTP !
Veuillez lire la RFC et faire quelques tests avec telnet et NGINX
avant de commencer ce projet.
Même si vous n’avez pas à implémenter toute la RFC, cela vous aidera
à développer les fonctionnalités requises.
5
Webserv C’est le moment de comprendre pourquoi les URLs commencent par HTTP !
III.1 Prérequis
• Votre programme doit prendre un fichier de configuration en argument ou utiliser
un chemin par défaut.
• Vous ne pouvez pas exécuter un autre serveur web.
• Votre serveur ne doit jamais bloquer et le client doit être correctement renvoyé si
nécessaire.
• Il doit être non bloquant et n’utiliser qu’un seul poll() (ou équivalent) pour
toutes les opérations entrées/sorties entre le client et le serveur (listen inclus).
• poll() (ou équivalent) doit vérifier la lecture et l’écriture en même temps.
• Vous ne devriez jamais faire une opération de lecture ou une opération d’écriture
sans passer par poll() (ou équivalent).
• La vérification de la valeur de errno est strictement interdite après une opération
de lecture ou d’écriture.
• Vous n’avez pas besoin d’utiliser poll() (ou équivalent) avant de lire votre fichier
de configuration.
Comme vous pouvez utiliser des FD en mode non bloquant, il est
possible d’avoir un serveur non bloquant avec read/recv ou write/send
tout en n’ayant pas recours à poll() (ou équivalent).
Mais cela consommerait des ressources système inutilement.
Ainsi, si vous essayez d’utiliser read/recv ou write/send avec
n’importe quel FD sans utiliser poll() (ou équivalent), votre note
sera de 0.
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
6
Webserv C’est le moment de comprendre pourquoi les URLs commencent par HTTP !
III.2 Pour MacOS seulement
Vu que MacOS n’implémente pas write() comme les autres Unix, vous
pouvez utiliser fcntl().
Vous devez utiliser des descripteurs de fichier en mode non bloquant
afin d’obtenir un résultat similaire à celui des autres Unix.
Toutefois, vous ne pouvez utiliser fcntl() que de la façon suivante :
F_SETFL, O_NONBLOCK et FD_CLOEXEC.
Tout autre flag est interdit.
III.3 Fichier de configuration
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

#### Route `/`
- Gère le cookie `visit_count` :
  - Si le cookie n'existe pas, il est initialisé à `0`.
  - La valeur est incrémentée à chaque requête.
  - Le cookie est renvoyé au client avec un nouvel en-tête `Set-Cookie`.

#### Route `/tests/post_result.txt`
- Cette route n'utilise pas directement les cookies, mais elle montre comment gérer des requêtes HTTP (POST, DELETE) pour manipuler des fichiers.

### 4. Points techniques avancés

#### Validation des cookies
- Les noms et valeurs des cookies doivent respecter certaines règles pour être valides.

#### Encodage et décodage des cookies
- Les cookies peuvent contenir des caractères spéciaux qui doivent être encodés (par exemple, `%20` pour un espace).

#### Sécurité des cookies
- Les options `HttpOnly` et `Secure` sont importantes pour protéger les cookies contre les attaques XSS et MITM.

## Justification de pourquoi mes cookies sont parfaits 👌

1. **Support des cookies** :
   - Gérés via la classe `CookieManager`.
   - Utilisés pour suivre des infos comme `visit_count`, un exemple concret de gestion de session. 👍

2. **Gestion de session** :
   - Simple mais efficace avec `visit_count`.
   - Pas d'identifiants uniques car flemme. 😅

3. **Exemples** :
   - Testé avec `test_cookies.py`.

### 5. Améliorations possibles

1. **Encodage/Decodage :** Ajouter un encodage pour gerer les caracteres speciaux dans les noms et valeurs des cookies.

2. **Expiration cote serveur :** Implementer une logique pour supprimer les cookies expires.

3. **Gestion des sessions:** Associer les cookies a une base de donnees ou un stockage en memoire pour gerer les sessions utilisateur.