<h1 align="center" style="color:#FF0000;">
   La logique est plus croise que l'arbre genealogique de sam
</h1>

<p align="center">
  <img src="https://cdn.intra.42.fr/users/74b82ba7e6ebf0bb551b411fd7d835e2/sheiles.jpg" alt="Image 1" width="45%" height="300px" style="border-radius: 10px;"/>
  <img src="https://github.com/Axeltheaxelotl/boite-a-foutre/blob/main/67d0591149b44087559381.gif?raw=true" alt="Image 2" width="45%" height="300px" style="border-radius: 10px;"/>
</p>

<p align="center">
  <img src="https://miro.medium.com/v2/resize:fit:4800/format:webp/1*qw6GeQe9tMULtH0ifqtL7Q.png" alt="Sam et sa cousine" style="width:915px; height:500px;"/>
</p>

<div>
  <h1 align="center"> Webserv dans ta grosse daronne</h1>
    <h3 align="left">1.Initialisation du Webserv:</h3>
     <p>
        fichiers de base, Structure.
     </p>
    <h3 align="left">2.Fichier de configuration:</h3>
    <p>
      - L'hote et le port ecouter.<br>
      - Les routes et les methodes HTTP autorisees.<br>
      - Configuration des pages d'erreur par default.<br>
      - Les repertoires a servir, les fichiers a lister et la gestion des fichiers uplaoudes.<br>
    </p>
    <h3 align="left">3.Gestion des descripteurs de fichier et de connexion:</h3>
      <p>
        - Le serveur doit ecouter surt plusieurs ports, accepter des connexions de clients et gerer des requetes HTTP<br>
        - select(), poll(), epoll() pour gerer plusieurs descripteurs de fichier sans blocage.
      </p>
    <h3 align="left">4.Gestion des requetes HTTP:</h3>
      <p>
        - Un parser pour analyser les requetes GET, POST, DELETE.<br>
        - Gerer les headers HTTP.<br>
        - Gerer les reponses HTTP et envoyer le bon code de status.<br>
        - Traiter les corps de requetes pour POST.<br>
        - Mettre en place un "Mecanisme" pour gerer les redirections HTTP et les revoies sur des fichiers demandes.
      </p>
    <h3 align="left">5.Gestion des erreurs HTTP:</h3>
      <p>
        - Prevoire des pages d'erreur par default.<br>
        - Si une ressource n'est pas trouvee, renvoyer 404 plus page.

