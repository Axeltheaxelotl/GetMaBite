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