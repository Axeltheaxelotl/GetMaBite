<div>
  <h1 align="center">Comment que j ai fait le <strong>"epoll"</strong></h1>
  <p align="left">
  <strong><h2>1. Constructeur et destructeur :</h2></strong><br>
  
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

<h2>2. Configuration des serveurs :</h2>

    void EpollClasse::setupServers(std::vector<ServerConfig> servers)
    {
        Logger::logMsg(LIGHTMAGENTA, CONSOLE_OUTPUT, "Setting up servers...");
        _servers = servers;

        for (std::vector<ServerConfig>::iterator it = _servers.begin(); it != _servers.end(); ++it)
        {
            it->setupServer();
            Logger::logMsg(LIGHTMAGENTA, CONSOLE_OUTPUT, "Server Created: %s", it->getServerName().c_str());

            epoll_event event;
            event.events = EPOLLIN | EPOLLET;
            event.data.fd = it->getFd();
            addToEpoll(it->getFd(), event);
        }
    }

<p>
&emsp; - <strong>_server :</strong> stocke la liste des serveurs configures.<br>
&emsp; - <strong>setupServer :</strong> configure chaque serveur (sockets, liaison a un port, et je c plus)<br>
&emsp; - <stong>epoll_envent :</strong> structure utilisee pour specifier les evenements a surveiller <br> 
&emsp; &emsp; * <strong>EPOLLIN :</strong> surveille les evenements de lecture.<br>
&emsp; &emsp; * <strong>EPOLLET :</strong> active le mode "edge-triggered" pour une meilleure performance.<br>
&emsp; - <strong>addToEpoll</strong> ajoute le descripteur de fichier a epoll ou le met a jour si necessaire, Si un descripteur a echoue a etre ajoute, un message d erreur s affiche et le programme se termine
</p>

<h2>3. Boucle principale :</h2>

<h3>Serveur Run</h3>

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
                else if (_events[i].events & EPOLLOUT)
                {
                    handleWrite(_events[i].data.fd);
                }
                else if (_events[i].events & (EPOLLERR | EPOLLHUP))
                {
                    handleError(_events[i].data.fd);
                }
            }
        }
    }

<p>
&emsp; - <strong>epoll_wait :</strong> Attend les evenements sur les descripteurs surveilles.<br>
&emsp; &emsp; * <strong>_events :</strong> Tableau contenant les evenements detectes.<br>
&emsp; &emsp; * <strong>MAX_EVENTS :</strong> Nombre maximum d evenements a traiter en une seule fois.<br>
&emsp; &emsp; * <strong>-1 :</strong> Attend indefiniment jusqu a ce qu un evenement se produise.<br>
&emsp; <strong>- Gestion des evenements :</strong><br>
&emsp; &emsp; <strong>* EPOLLIN :</strong> Si un evenement de lecture est detecte:<br>
&emsp; &emsp; &emsp; * Si le descripteur est un serveur (isServerFd), une nouvelle connexion est acceptee (acceptConnection).<br>
&emsp; &emsp; &emsp; * Sinon, une requete client est traitee (handleRequest).<br>
&emsp; &emsp; <strong>EPOLLOUT :</strong> Si un evenement d'ecriture est detecte, une reponse est envoyee

<h2>4. Ajouter un descripteur a epoll</h2>

<h3>Methode : <strong>addToEpoll</strong></h3>


    void EpollClasse::addToEpoll(int fd, epoll_event &event)
    {
        // Supprimer le FD s'il existe déjà
        epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, fd, NULL);

        // Ajouter le FD à epoll
        if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, fd, &event) == -1)
        {
            Logger::logMsg(RED, CONSOLE_OUTPUT, "Epoll add error: %s", strerror(errno));
            exit(1);
        }
        if (fd > _biggest_fd)
        {
            _biggest_fd = fd;
        }
    }

&emsp; <strong>- epoll_ctl</strong>
&emsp; &emsp; * <strong>EPOLL_CTL_DEL :</strong> Supprime le descripteur de fichier (FD) s'il est deja enregistre dans epoll. ca evite les doublons.<br>