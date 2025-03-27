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
  <h1 align="center"> WEBSERV DANS TA GROSSE DARONNE</h1>
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
      </p>
    <h3 align="left">6.Bonus CGI:</h3>
      <p>
        je sais pas si c'est Simon le gay qui le fait.
      </p>

<div>
  <table align="center" style="width: 100%; table-layout: fixed;">
    <tr>
      <td>
        <h2 align="center">REPARTION DU PROJET COMME CA SIMON FAIT TOUT</h2>
         <p>
            <h4>P1.Gestion des requetes HTTP et reponse:</h4><br>
            - Initialisation du serveur.<br>
            &emsp;- fichiers de base et configurez le projet.<br>
            &emsp;- ecrire le code  de gestion des connections client.<br>
            - Traitement des requetes HTTP.<br>
            - Tests les methodes GET et reponses aux requetes simple.
            <h4>P2.fichier de configuration et gestion des routes:</h4><br>
            - Parser fichier de configuration.<br>
            - Assurer la gestion des pages d'erreur personnalisees depuis la configuration<br>
            - Gestion des methodes POST et DELETE<br>
            &emsp;- Implementer la gestion des donnees envoyees dans un corps des requetes HTTP.<br>
            &emsp;- Gerer les routes pour les methodes POST et DELETE et tester le televersement des fichiers.<br>
            - Gestion du CGI:<br>
            &emsp; - Gestion des scripts CGI
      </td>
      <td style="vertical-align: top;">
        <h2 align="center">STRUCTURE DE MERDE</h2>

    webserv/
    ├── src/
    │   ├── main.cpp
    │   ├── serveur.cpp
    │   ├── request.cpp
    │   ├── reponse.cpp
    │   ├── configParser.cpp
    │   ├── Utils.cpp
    │   └── CGI.cpp
    ├── includes/
    ├── www/
    ├── config/
    │   ├── default.conf
    │   └── error_pages/
    │       ├── 404.html
    │       ├── 500.html
    │       └── ...
    └── Makefile

  <img align="center" src="https://i.pinimg.com/736x/35/01/9c/35019c811ce4c988b3d41ae5ce8645a7.jpg" alt="Sam et sa cousine" style="width:550px; height:450px;"/>
    
  </td>
  </tr>
  </table>
</div>

---

<div>
  <h2 align="center">Dit pere castor 🦫 c quoi "epoll":</h2>
    <p align="left">
      <strong>1. /  </strong><strong>"epoll"</strong> scanne pas tous les descripteurs de fichiers a chaque appel, il utilise un truc base sur des "evenements".
    </p>
    <p>
      <strong>2. /  </strong>Il gere des miliers de descripteurs de fichier avec une surcharge minimal.
    </p>
    <p>
      <strong>3. / </strong>Non bloquand : permet de gerer plusieurs connexions  simultanement sans etre bloque par une seule operation d'I/O.
    </p>
    <p>
    <h3 align="left"> Fonctionnement de epoll :</h3>
    <p>
      <strong>1. /   </strong><strong>"epoll_create1"</strong> cree une instance epoll
    </p>
    <p>
      <strong>2. /   </strong><strong>"epoll_ctl"</strong> ajoute des descriprteurs de fichier a surveiller.
    </p>
    <p>
      <strong>3. /   </strong><strong>"epoll_wait"</strong> attend les evenements sur les descripteurs ajoutes.
    </p>
    <p>
      <strong>4. /   </strong> Les evenements dont traites en cosequence (accepter une nouvelle connexion, lire des donnees, etc ...).
    </p>
    
  <h3>EXEMPLE:</h3>
  



    ‎ 
    EpollClasse::EpollClasse()
    {
      _epoll_fd = epoll_create1(0);
      if (_epoll_fd == -1)
      {
        Logger::logMsg(RED, CONSOLE_OUTPUT, "Epoll creation error: %s", strerror(errno));
        exit(1);
      }
      _biggest_fd = 0;
    }

    void EpollClasse::serverRun()
    {
      while (true)
      {
        int event_count = epoll_wait(_epoll_fd, _events, MAX_EVENTS, -1);
        if (event_count == -1)
        {
            Logger::logMsg(RED, CONSOLE_OUTPUT, "Epoll wait error: %s", strerror(errno));
            exit(1);
        }
        for (int i = 0; i < event_count; ++i)
        {
            if (_events[i].events & EPOLLIN)
            {
                if (isServerFd(_events[i].data.fd))
                {
                    acceptConnection(_events[i].data.fd);
                }
                else
                {
                    handleRequest(_events[i].data.fd);
                }
             }
          }
       }
    }

<p>
  <strong>"epoll"</strong> permet de creer des serveurs non-bloquants capables de gerer plusieurs de nombreuses connexions simultanement avec une performance elevee.
</p>

<div>
  <h2 align="center">Inshallah 🕋🤲 epoll c plus mieux</h2>
  <p align="left">
  <strong><h3><a href="https://man7.org/linux/man-pages/man2/select.2.html" target="_blank" >🔗select :👎</a></h3></strong>
  &emsp;- Limite a 1024 descripteurs, nul pour un grand nombre de descripteurs.<br>
  &emsp;- Scanne tous les descripteurs a chaque appel.<br>
  &emsp;- C de la grosse demerde
  </p>

  <p>
  <strong><h3><a href="https://man7.org/linux/man-pages/man2/poll.2.html" target="_blank" >🔗poll :👎</a ></h3></strong>
  &emsp; - Pas de limite fixe sur le nombre de descripteurs<br>
  &emsp; - Comme le <strong>"select"</strong> de merde, il scanne tous les descripteurs a chaque appel. <br>
  &emsp; - C comme le select c de la merde qu'ils retournes chez eux
  </p>

  <p>
  <strong><h3><a href="https://man7.org/linux/man-pages/man7/epoll.7.html" target="_blank">🔗epoll :👍</a></h3></strong>
  &emsp; - Utilise le truc base sur les evenements, bas besoin de scanner tous les descripteurs.<br>
  &emsp; - Gere des miliers de descripteurs avec une surcharge minimal masterchiasse.<br>
  &emsp; - Permet de gerer plusieurs connexions simultanement sans blocage.<br>
  &emsp; - TOUT EST POSSIBLE AVEC LA CARTE KIWI.
  </p>

<div align="center">
  <img src="https://i.pinimg.com/736x/9b/a8/79/9ba87924a08294740afbb489b74de37c.jpg" alt="Sam et sa cousine" style="width:6750px; height:350px;"/>
</div>

<div>
  <h2 align="center">Comment que j ai fait le <strong>"epoll"</strong></h2>
  <p align="left">
  <strong><h3>1. Constructeur et destructeur :</h3></strong><br>
  
  <h3>Constructeur :</h3>

    EpollClasse::EpollClasse()
    {
      _epoll_fd = epoll_create1(0);
      if (_epoll_fd == -1)
      {
          Logger::logMsg(RED, CONSOLE_OUTPUT, "Epoll creation error: %s", strerror(errno));
           exit(1);
      }
      _biggest_fd = 0;
    }

  . epoll_created1(0) : cree une instance epoll. Elle retourne un descripteur de fichier _epoll_fd qui et utilise pour surveiller plusieurs descripteurs.<br>

  . <strong>Gestion des erreurs :</strong> si echoue, retourne un message d erreur et ntm le programme.<br>

  . _biggest_fd : stocke le plus grand descripteur de fichier parceque apartement utile pour optimisation que j ai pas fait<br>

<h3>Destructeur :</h3>

un destructeur c un destructeur<br>




