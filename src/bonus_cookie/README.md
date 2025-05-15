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

### 5. Améliorations possibles
- **Gestion des sessions avancée** :
  - Utiliser un cookie `session_id` pour identifier les utilisateurs de manière unique.
  - Associer chaque `session_id` à des données côté serveur (par exemple, dans une base de données ou une structure en mémoire).
- **Encodage/décodage des cookies** :
  - Ajouter des fonctions pour gérer les caractères spéciaux dans les noms et valeurs des cookies.
- **Expiration des cookies côté serveur** :
  - Implémenter une logique pour supprimer les cookies expirés côté serveur.

## Justification de pourquoi mes cookies sont parfaits 👌

1. **Support des cookies** :
   - Gérés via la classe `CookieManager`.
   - Utilisés pour suivre des infos comme `visit_count`, un exemple concret de gestion de session. 👍

2. **Gestion de session** :
   - Simple mais efficace avec `visit_count`.
   - Pas d'identifiants uniques car flemme. 😅

3. **Exemples** :
   - Testé avec `test_cookies.py`.


