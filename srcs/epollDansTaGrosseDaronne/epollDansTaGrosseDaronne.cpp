#include "epollDansTaGrosseDaronne.hpp"

// constructeur du epollDansTaGrosseDaronne, epoll_fd
epollDansTaGrosseDaronne::epollDansTaGrosseDaronne()
{
    _epoll_fd = epoll_create1(0); // Crée une instance d'epoll
    if (_epoll_fd == -1)
    {
        Logger::logMsg(RED, CONSOLE_OUTPUT, "Epoll creation error: %s", strerror(errno));
        exit(1);
    }
    _biggest_fd = 0; // plus grand descripteur de fichier
}

epollDansTaGrosseDaronne::~epollDansTaGrosseDaronne()
{
    close(_epoll_fd); // ferme epoll_fd quand le serveur est arrêté
}

// initialise les serveurs à partir de la conf
void epollDansTaGrosseDaronne::setupServers(std::vector<ServerConfig> servers)
{
    Logger::logMsg(LIGHTMAGENTA, CONSOLE_OUTPUT, "Setting up servers...");
    _servers = servers;

    // pour chaque serveur dans la liste de configuration
    for (auto &it : _servers)
    {
        it.setupServer(); // chaque serveur
        Logger::logMsg(LIGHTMAGENTA, CONSOLE_OUTPUT, "Server Created: %s", it.getServerName().c_str());

        // Ajouter chaque serveur à epoll pour la surveillance de la lecture
        epoll_event event;
        event.events = EPOLLIN | EPOLLET; // pour surveiller la lecture non-bloquante
        event.data.fd = it.getFd(); // descripteur de fichier du serveur
        addToEpoll(it.getFd(), event);
    }
}

// boucle principale du serveur, gérant les événements avec epoll
void epollDansTaGrosseDaronne::serverRun()
{
    while (true)
    {
        // attendre les événements epoll
        int event_count = epoll_wait(_epoll_fd, _events, MAX_EVENTS, -1); // blocage en attente d'un événement

        if (event_count == -1)
        {
            Logger::logMsg(RED, CONSOLE_OUTPUT, "Epoll wait error: %s", strerror(errno));
            exit(1);
        }

        // gestion des événements
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

void epollDansTaGrosseDaronne::addToEpoll(int fd, epoll_event &event)
{
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

bool epollDansTaGrosseDaronne::isServerFd(int fd)
{
    for (auto &server : _servers)
    {
        if (server.getFd() == fd)
        {
            return true;
        }
    }
    return false;
}

void epollDansTaGrosseDaronne::acceptConnection(int server_fd)
{
    struct sockaddr_in client_address;
    socklen_t addrlen = sizeof(client_address);
    int client_fd = accept(server_fd, (struct sockaddr *)&client_address, &addrlen);
    if (client_fd == -1)
    {
        Logger::logMsg(RED, CONSOLE_OUTPUT, "Accept error: %s", strerror(errno));
        return;
    }
    setNonBlocking(client_fd);
    epoll_event event;
    event.events = EPOLLIN | EPOLLET;
    event.data.fd = client_fd;
    addToEpoll(client_fd, event);
    Logger::logMsg(GREEN, CONSOLE_OUTPUT, "Accepted connection from %s:%d", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));
}

void epollDansTaGrosseDaronne::handleRequest(int client_fd)
{
    char buffer[BUFFER_SIZE];
    int bytes_read = read(client_fd, buffer, BUFFER_SIZE);
    if (bytes_read <= 0)
    {
        close(client_fd);
        return;
    }
    // Traitez la requête HTTP ici
    // ...
    close(client_fd);
}