<h1 align="center"><u>Pourquoi mon epoll et parfait</u></h1>

<p>
<h3><b><u>Oui mon epoll et parfaitement parfait :</u></b></h3>
Le subject demande:<br>
&emsp; - un serveur non bloquant.<br>
&emsp; - un seule epoll pour toutes les operations E/S.<br>
&emsp; - ne jamais faire de read/write sans passer par epoll.<br>
&emsp; - gere plusieurs sockets (serveur et client) en meme temps.<br>
</p>
<p>
de ce que j'ai compris epoll est l API linux la plus efficasse.<br>
Utilisee bien epoll_create1, epoll_ctl, epoll_wait, ca ce qui faut.<br>
</p>

---

<h3><u>Utilisation de epoll :</u></h3>

<p>
<h4>Initialisation epoll :</h4>
Classe EpollClasse :<br>
&emsp; - creer un descripteur epoll avec epoll_create1.<br>
&emsp; - ajoute des sockets serveurs (ceux qui ecoutent) a la liste a surveillee avec epoll_ctl<br>
&emsp; (operation EPOLL_CTL_ADD) en mode non-bloquant.<br>
</p>

<p>
<h4>Boucle principale :</h4>
while(1) ou j'appelles epoll_wait :<br>
&emsp; - fonction qui bloque jusqu'a ce qu'un ou plusieurs sockets soient prets (nouvelle connection,<br>
&emsp; donnees a lire, etc...).<br>
&emsp; - retoune un tableau d'evenements (struct epoll_event).<br>
</p>

<p>
<h4>Traitement des evenements :</h4>
&emsp; - sockets serveur pret : c'est une nouvelle connection. Tu acceptes avec epoll_accept(), mets le socket<br>
&emsp; en non-bloquant, et ajoute a epoll.<br>
&emsp; - socket client pret a lire: tu lis les donnees (requete http), si la requete est complete, tu la<br>
&emsp; traites et prepares la reponse.<br>
&emsp; - socket client pret a ecrire: envoie reponse http.<br>
</p>