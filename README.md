<h1 align="center" style="color:#FF0000;">
   La logique est plus croise que l'arbre genealogique de sam
</h1>

<p align="center">
  <img src="https://cdn.intra.42.fr/users/74b82ba7e6ebf0bb551b411fd7d835e2/sheiles.jpg" alt="Image 1" width="45%" height="300px" style="border-radius: 10px;"/>
  <img src="https://github.com/Axeltheaxelotl/boite-a-foutre/blob/main/67d0591149b44087559381.gif?raw=true" alt="Image 2" width="45%" height="300px" style="border-radius: 10px;"/>
</p>

<table align="center" style="width: 100%; max-width: 600px; border-collapse: collapse; padding: 0 20px;">
  <tr>
    <td style="text-align: left; padding: 10px; border: 1px solid #ddd;">
      <p>---
#### **C'est quoi la RFC 2616 ?**
La **RFC 2616** définit le protocole **HTTP/1.1**, qui est utilisé pour la communication entre les **navigateurs** (les clients) et les **serveurs web**. HTTP/1.1 est la version la plus courante du protocole, et il est responsable des échanges de données entre le client et le serveur.

https://miro.medium.com/v2/resize:fit:4800/format:webp/1*qw6GeQe9tMULtH0ifqtL7Q.png

#### **Ce qu'il faut absolument comprendre :**

1. **Les méthodes HTTP :**  
   Il faut bien saisir comment fonctionnent les principales méthodes HTTP, comme **GET**, **POST**, **PUT**, **DELETE**, **HEAD**, **OPTIONS**. Chacune d'elles a un rôle spécifique dans l’interaction avec le serveur.

2. **Les codes de statut HTTP :**  
   Quand un serveur répond à une requête, il renvoie un code qui donne une idée de ce qui s’est passé. Par exemple, **200 OK** signifie que tout s'est bien passé, **404 Not Found** indique que la page n'existe pas, et **500 Internal Server Error** signifie qu’il y a eu un problème sur le serveur.

3. **Les en-têtes HTTP :**  
   Les en-têtes HTTP, comme **Content-Type**, **Content-Length**, ou **Host**, transportent des informations importantes sur la requête et la réponse. Apprendre à les gérer est crucial pour bien faire communiquer ton serveur avec le client.

4. **La structure des requêtes et des réponses HTTP :**  
   Il faut comprendre comment les requêtes HTTP sont envoyées et comment le serveur répond. Cela te permettra de gérer correctement le flux d'information.

5. **La gestion des connexions HTTP :**  
   Il y a une différence entre **keep-alive** (pour garder la connexion ouverte entre plusieurs requêtes) et **close** (pour fermer la connexion après chaque requête). C’est important pour gérer la persistance des connexions.

#### **Pourquoi c'est important ?**
Tout ça te permet de construire un serveur qui interagit correctement avec les clients. Comprendre ces bases te donne aussi un meilleur aperçu de la communication entre un client et un serveur web.

---

### **Faire des tests avec Telnet**  
===========================

#### **Pourquoi utiliser Telnet ?**
**Telnet** te permet de simuler des requêtes HTTP sans avoir besoin de passer par un navigateur. C’est un excellent moyen de comprendre comment les requêtes et réponses HTTP fonctionnent en réalité, et ça te donne un aperçu très pratique de la communication entre client et serveur.

#### **Comment faire des tests avec Telnet :**

1. **Se connecter au serveur :**  
   Pour se connecter à un serveur HTTP, tu peux utiliser cette commande :
      GET /index.html HTTP/1.1 Host: localhost

![Telnet Command Example](https://example.com/telnet_command_image.png)  
_Exemple d'une requête GET via Telnet_

3. **Explorer les réponses HTTP :**  
Regarde bien la structure des réponses que le serveur renvoie, les en-têtes qu’il utilise et comment il renvoie les données.

![Telnet Response Example](https://example.com/telnet_response_image.png)  
_Exemple de réponse HTTP via Telnet_

4. **Tester d’autres méthodes HTTP :**  
Tu peux aussi tester d’autres méthodes comme **POST** ou **PUT** pour voir comment le serveur réagit selon les types de requêtes.

#### **Pourquoi c'est utile ?**
Les tests avec Telnet te permettent de mieux comprendre la structure des requêtes et des réponses HTTP. Ça te prépare à gérer les protocoles HTTP dans ton propre serveur.

---

### **Faire des tests avec NGINX**  
=========================

#### **Pourquoi tester avec NGINX ?**
**NGINX** est un serveur HTTP super performant. Il est largement utilisé en production, donc le tester te permettra de voir comment un serveur bien rôdé gère les requêtes. C’est aussi une excellente référence pour t’assurer que ton serveur fonctionne correctement.

#### **Comment utiliser NGINX :**

1. **Installer NGINX :**  
Si tu es sur **Linux (Debian/Ubuntu)**, tu peux installer NGINX avec la commande suivante :
   sudo apt install nginx

2. **Démarrer NGINX :**  
Pour démarrer le serveur, utilise cette commande :
   sudo systemctl start nginx

3. **Tester NGINX :**  
Vérifie qu'il fonctionne bien en allant sur **http://localhost** dans ton navigateur ou en utilisant **curl** :
   curl http://localhost


4. **Analyser les logs :**  
Les logs d’NGINX te permettent de voir comment il gère les requêtes. Pour les afficher en temps réel, tu peux utiliser :
   sudo tail -f /var/log/nginx/access.log


![NGINX Logs](https://example.com/nginx_logs_image.png)  
_Exemple de logs NGINX_

5. **Simuler des erreurs :**  
Tu peux aussi tester des erreurs en accédant à une page qui n’existe pas, par exemple **http://localhost/404page**, et observer comment le serveur réagit.

![404 Error Example](https://example.com/404_error_image.png)  
_Exemple d'une erreur 404 sur NGINX_

#### **Pourquoi c'est important ?**
Utiliser NGINX t’aide à comparer ton serveur avec un serveur éprouvé et performant. Cela te permet aussi de comprendre comment un serveur bien configuré gère les requêtes et les erreurs.

---

### **Concepts supplémentaires à maîtriser**  
==================================

1. **Gestion des erreurs HTTP :**
Tu dois comprendre comment gérer les **codes d’erreur HTTP** comme **404 Not Found** ou **500 Internal Server Error**, et savoir comment générer des pages d'erreur personnalisées.

2. **Header Parsing :**
Il est essentiel de savoir **parser les en-têtes HTTP** pour extraire des informations importantes comme le type de contenu (**Content-Type**) et la longueur du message (**Content-Length**).

3. **Sécurisation du serveur :**
La sécurité du serveur est primordiale. Il faut comprendre comment gérer des en-têtes de sécurité comme **Content-Security-Policy** ou **X-Frame-Options**.

4. **Réseau et Ports :**
Enfin, tu dois maîtriser le fonctionnement des **ports réseau** (par exemple, le port 80 pour HTTP) et comprendre comment ton serveur interagit avec le réseau.

---

### **Conclusion**  
===========

Avant de démarrer ton projet **webserv**, il est vraiment important de :
- Lire et comprendre la **RFC 2616** pour bien maîtriser le protocole HTTP.
- Faire des tests avec **Telnet** pour comprendre comment les requêtes et les réponses HTTP fonctionnent.
- Tester avec **NGINX** pour avoir une référence solide et t’assurer que ton serveur fonctionne comme prévu.

Ces étapes te donneront une bonne base pour ton projet, en t'aidant à comprendre en profondeur le fonctionnement du protocole HTTP et à comparer ton serveur avec un serveur déjà performant comme **NGINX**.

---</p>
    </td>
    <td style="text-align: right; padding: 10px; border: 1px solid #ddd;">
      <p>## Priorités


| Priorité  | Tâche                                  | Description |
|-----------|----------------------------------------|-------------|
| 1         | **Gestion des requêtes HTTP**          | Implémenter la réception et le traitement des requêtes HTTP (`GET`, `POST`, `DELETE`). |
| 2         | **Serveur non-bloquant avec `poll()`**  | Assurer que le serveur peut gérer des connexions simultanées sans se bloquer. |
| 3         | **Fichier de configuration**           | Permettre la configuration des serveurs, routes, et méthodes HTTP. |
| 4         | **Gestion des réponses HTTP**          | Générer des réponses HTTP correctes, y compris les pages d'erreur par défaut. |
| 5         | **Exécution de CGI**                   | Implémenter l'exécution de scripts CGI (ex : PHP, Python). |
| 6         | **Téléversement de fichiers**          | Ajouter la fonctionnalité pour accepter et gérer les uploads de fichiers. |
| 7         | **Test de résistance**                 | Vérifier que le serveur fonctionne sous haute charge sans se planter. |

---

```plaintext
webserv/
│
├── src/
│   ├── main.cpp              # Point d'entrée du serveur
│   ├── server.cpp            # Logique du serveur HTTP
│   ├── request.cpp           # Gestion des requêtes HTTP
│   ├── response.cpp          # Gestion des réponses HTTP
│   ├── config.cpp            # Gestion du fichier de configuration
│   ├── utils.cpp             # Fonctions utilitaires (ex: gestion des erreurs)
│   └── cgi.cpp               # Gestion des scripts CGI
│
├── include/
│   ├── server.hpp            # Déclaration de la classe Server
│   ├── request.hpp           # Déclaration de la classe Request
│   ├── response.hpp          # Déclaration de la classe Response
│   ├── config.hpp            # Déclaration de la classe Config
│   └── utils.hpp             # Déclarations utilitaires
│
├── Makefile                  # Fichier pour la compilation
├── config_example.txt        # Exemple de fichier de configuration
└── README.md                 # Fichier de documentation
```
</p>
    </td>
  </tr>
</table>