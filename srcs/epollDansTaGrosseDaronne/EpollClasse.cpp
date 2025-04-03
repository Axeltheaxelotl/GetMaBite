#include "EpollClasse.hpp"
#include <cerrno>
#include <cstdlib>
#include <fstream>
#include <cstring>
#include "../Logger/Logger.hpp"
#include "../routes/AutoIndex.hpp"

// Constructeur
EpollClasse::EpollClasse()
{
    _epoll_fd = epoll_create1(0);
    if (_epoll_fd == -1)
    {
        Logger::logMsg(RED, CONSOLE_OUTPUT, "Epoll creation error: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }
    _biggest_fd = 0;
}

// Destructeur
EpollClasse::~EpollClasse()
{
    if (_epoll_fd != -1)
    {
        close(_epoll_fd);
    }
}

// Configuration des serveurs
void EpollClasse::setupServers(std::vector<ServerConfig> servers, const std::vector<Server> &serverConfigs)
{
    Logger::logMsg(LIGHTMAGENTA, CONSOLE_OUTPUT, "Setting up servers...");
    _servers = servers;
    _serverConfigs = serverConfigs;

    for (std::vector<ServerConfig>::iterator it = _servers.begin(); it != _servers.end(); ++it)
    {
        it->setupServer();
        Logger::logMsg(LIGHTMAGENTA, CONSOLE_OUTPUT, "Server Created: %s", it->getServerName().c_str());

        epoll_event event;
        event.events = EPOLLIN | EPOLLET; // Lecture et mode edge-triggered
        event.data.fd = it->getFd();

        addToEpoll(it->getFd(), event);
    }
}

// Boucle principale
void EpollClasse::serverRun()
{
    while (true)
    {
        int event_count = epoll_wait(_epoll_fd, _events, MAX_EVENTS, -1);
        if (event_count == -1)
        {
            Logger::logMsg(RED, CONSOLE_OUTPUT, "Epoll wait error: %s", strerror(errno));
            exit(EXIT_FAILURE);
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
            else if (_events[i].events & (EPOLLERR | EPOLLHUP))
            {
                handleError(_events[i].data.fd);
            }
        }
    }
}

// Ajouter un descripteur à epoll
void EpollClasse::addToEpoll(int fd, epoll_event &event)
{
    if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, fd, &event) == -1)
    {
        Logger::logMsg(RED, CONSOLE_OUTPUT, "Epoll add error (FD: %d): %s", fd, strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (fd > _biggest_fd)
    {
        _biggest_fd = fd;
    }
}

// Vérifier si le FD est un serveur
bool EpollClasse::isServerFd(int fd)
{
    for (std::vector<ServerConfig>::iterator it = _servers.begin(); it != _servers.end(); ++it)
    {
        if (it->getFd() == fd)
        {
            return true;
        }
    }
    return false;
}

// Accepter une connexion
void EpollClasse::acceptConnection(int server_fd)
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
    event.events = EPOLLIN | EPOLLET; // Lecture et mode edge-triggered
    event.data.fd = client_fd;

    addToEpoll(client_fd, event);
    Logger::logMsg(GREEN, CONSOLE_OUTPUT, "Accepted connection from %s:%d",
                   inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));
}

// Résoudre le chemin demandé
std::string EpollClasse::resolvePath(const Server &server, const std::string &requestedPath)
{
    if (requestedPath == "/")
        return server.root + "/" + server.index;

    return server.root + requestedPath;
}

// Gérer une requête client
void EpollClasse::handleRequest(int client_fd)
{
    char buffer[BUFFER_SIZE];
    int bytes_read = read(client_fd, buffer, BUFFER_SIZE);
    if (bytes_read <= 0)
    {
        close(client_fd);
        return;
    }

    buffer[bytes_read] = '\0';
    std::string request(buffer);

    // Extraire le chemin du fichier demandé
    std::string requestedPath = "/";
    size_t pos = request.find("GET ");
    if (pos != std::string::npos)
    {
        size_t start = pos + 4; // Après "GET "
        size_t end = request.find(" ", start);
        requestedPath = request.substr(start, end - start);
    }

    // Résoudre le chemin en fonction du serveur
    std::string filePath = resolvePath(_serverConfigs[0], requestedPath); // Exemple avec le premier serveur

    // Ouvrir et lire le fichier demandé
    std::ifstream file(filePath.c_str());
    if (file)
    {
        std::string content((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());
        std::string response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n" + content;
        send(client_fd, response.c_str(), response.size(), 0);
    }
    else
    {
        // Si le fichier n'existe pas, retourner une erreur 404
        std::string errorResponse = "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\n\r\n"
                                    "<html><body><h1>404 Not Found</h1></body></html>";
        send(client_fd, errorResponse.c_str(), errorResponse.size(), 0);
    }

    close(client_fd);
}

// Gérer les erreurs
void EpollClasse::handleError(int fd)
{
    Logger::logMsg(RED, CONSOLE_OUTPUT, "Error on FD: %d", fd);
    close(fd);
}

// Définir un FD en mode non bloquant
void EpollClasse::setNonBlocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
    {
        Logger::logMsg(RED, CONSOLE_OUTPUT, "fcntl F_GETFL error: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
    {
        Logger::logMsg(RED, CONSOLE_OUTPUT, "fcntl F_SETFL error: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }
}

/*
Modification wsh:
    - gestion des chemins "dynamique" Methode resolvePath utilise les parametres root et index pour construire le chemin
    complet.

    - Support les serveurs multiples, les configurations des serveurs sont stockees dans _serverConfigs comme simon
    me la gentillement propose

    - Verification des erreurs pour toutes les operations CRITIQUES!!!

    - Autoindex mtn je peut integrer AutoIndex::generateAutoIndexPage pour generer une page d'index si necessaire.
*/