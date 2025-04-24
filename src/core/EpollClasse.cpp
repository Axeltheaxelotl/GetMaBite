#include "EpollClasse.hpp"
#include <cerrno>
#include <cstdlib>
#include <fstream>
#include <cstring>
#include <sstream>
#include <sys/stat.h>
#include "../utils/Logger.hpp"
#include "../routes/AutoIndex.hpp"
#include "../utils/Utils.hpp"

// Fonction utilitaire pour convertir size_t en string (compatible C++98)
static std::string sizeToString(size_t value)
{
    std::ostringstream oss;
    oss << value;
    return oss.str();
}

// Utilitaire pour joindre deux chemins sans double slash
static std::string joinPath(const std::string& left, const std::string& right) {
    if (left.empty()) return right;
    if (right.empty()) return left;
    if (left[left.size() - 1] == '/' && right[0] == '/')
        return left + right.substr(1);
    if (left[left.size() - 1] != '/' && right[0] != '/')
        return left + "/" + right;
    return left + right;
}

// Utilitaire pour éviter le doublon de dossier (ex: /tests/tests/)
static std::string smartJoinRootAndPath(const std::string& root, const std::string& path) {
    // Si root se termine par un dossier (ex: /tests/) et path commence par ce dossier (ex: /tests/...), on évite le doublon
    size_t lastSlash = root.find_last_of('/', root.length() - 2); // ignore le slash final
    std::string rootDir = root.substr(lastSlash + 1, root.length() - lastSlash - 2); // nom du dossier root sans slash
    std::string prefix = "/" + rootDir + "/";
    if (rootDir.length() > 0 && path.find(prefix) == 0) {
        return joinPath(root, path.substr(prefix.length()));
    }
    // Sinon, comportement normal
    return joinPath(root, path[0] == '/' ? path.substr(1) : path);
}

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
        Logger::logMsg(LIGHTMAGENTA, CONSOLE_OUTPUT, "Server Created on %s:%d", 
                      it->getHost().c_str(), it->getPort());

        // Log des informations de configuration
        Logger::logMsg(GREEN, CONSOLE_OUTPUT, "Root directory: %s", _serverConfigs[0].root.c_str());
        Logger::logMsg(GREEN, CONSOLE_OUTPUT, "Index file: %s", _serverConfigs[0].index.c_str());

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
    // Si le chemin demandé est la racine, renvoyer le fichier index
    if (requestedPath == "/")
    {
        // Si index est déjà un chemin absolu, le retourner directement
        if (!server.index.empty() && server.index[0] == '/')
            return server.index;
        // Sinon, concaténer root + index proprement
        return joinPath(server.root, server.index);
    }

    // Pour tout autre chemin, vérifier si une location correspond
    for (std::vector<Location>::const_iterator it = server.locations.begin(); 
         it != server.locations.end(); ++it)
    {
        if (requestedPath.find(it->path) == 0)
        {
            // Si un alias est défini, l'utiliser
            if (!it->alias.empty())
            {
                return joinPath(it->alias, requestedPath);
            }
            // Sinon utiliser le root de la location ou du serveur
            std::string root = !it->root.empty() ? it->root : server.root;
            return smartJoinRootAndPath(root, requestedPath.substr(1));
        }
    }

    // Si aucune location ne correspond, utiliser le root du serveur
    return smartJoinRootAndPath(server.root, requestedPath);
}

// Gérer une requête client
void EpollClasse::handleRequest(int client_fd)
{
    char buffer[BUFFER_SIZE];
    std::string request;
    
    int bytes_read = read(client_fd, buffer, BUFFER_SIZE - 1);
    if (bytes_read <= 0)
    {
        close(client_fd);
        return;
    }

    buffer[bytes_read] = '\0';
    request = buffer;

    // Parser la requête HTTP
    std::string method, path, protocol;
    std::istringstream requestStream(request);
    requestStream >> method >> path >> protocol;

    if (path.empty())
        path = "/";

    // On utilise le premier serveur pour l'instant (à améliorer pour gérer plusieurs serveurs)
    const Server& server = _serverConfigs[0];

    // Utiliser resolvePath pour obtenir le chemin réel
    std::string resolvedPath = resolvePath(server, path);
    Logger::logMsg(GREEN, CONSOLE_OUTPUT, "Resolved path: %s", resolvedPath.c_str());

    // Vérifier si le fichier existe
    struct stat file_stat;
    if (::stat(resolvedPath.c_str(), &file_stat) == 0)
    {
        // Le fichier existe, on le traite selon la méthode
        if (method == "GET")
        {
            handleGetRequest(client_fd, resolvedPath, server);
        }
        else if (method == "POST")
        {
            handlePostRequest(client_fd, request, resolvedPath);
        }
        else if (method == "DELETE")
        {
            handleDeleteRequest(client_fd, resolvedPath);
        }
        else
        {
            // Méthode non supportée
            std::string errorContent = "<html><body><h1>405 Method Not Allowed</h1></body></html>";
            std::string response = "HTTP/1.1 405 Method Not Allowed\r\n"
                                "Content-Type: text/html\r\n"
                                "Content-Length: " + sizeToString(errorContent.size()) + "\r\n\r\n"
                                + errorContent;
            sendResponse(client_fd, response);
        }
    }
    else
    {
        // Fichier non trouvé
        Logger::logMsg(RED, CONSOLE_OUTPUT, "File not found: %s", resolvedPath.c_str());
        std::string errorContent = "<html><body><h1>404 Not Found</h1></body></html>";
        std::string response = "HTTP/1.1 404 Not Found\r\n"
                            "Content-Type: text/html\r\n"
                            "Content-Length: " + sizeToString(errorContent.size()) + "\r\n\r\n"
                            + errorContent;
        sendResponse(client_fd, response);
    }

    close(client_fd);
}

void EpollClasse::sendResponse(int client_fd, const std::string &response)
{
    size_t total_sent = 0;
    while (total_sent < response.size())
    {
        ssize_t sent = send(client_fd, response.c_str() + total_sent, response.size() - total_sent, 0);
        if (sent < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                continue;
            handleError(client_fd);
            return;
        }
        total_sent += sent;
    }
}

void EpollClasse::handleGetRequest(int client_fd, const std::string &filePath, const Server &server)
{
    // Vérifier si le fichier existe
    struct stat file_stat;
    Logger::logMsg(GREEN, CONSOLE_OUTPUT, "Trying to open file: %s", filePath.c_str());
    
    if (::stat(filePath.c_str(), &file_stat) == 0)
    {
        if (S_ISDIR(file_stat.st_mode))
        {
            // Si c'est un répertoire, vérifier l'autoindex
            bool autoindex = false;
            for (std::vector<Location>::const_iterator it = server.locations.begin();
                 it != server.locations.end(); ++it)
            {
                if (filePath.find(it->path) == 0)
                {
                    autoindex = it->autoindex;
                    break;
                }
            }

            if (autoindex)
            {
                std::string content = AutoIndex::generateAutoIndexPage(filePath);
                std::string response = "HTTP/1.1 200 OK\r\n"
                                     "Content-Type: text/html\r\n"
                                     "Content-Length: " + sizeToString(content.size()) + "\r\n\r\n" + 
                                     content;
                sendResponse(client_fd, response);
            }
            else
            {
                // Essayer d'utiliser le fichier index
                std::string indexPath = filePath;
                if (indexPath[indexPath.length() - 1] != '/')
                    indexPath += "/";
                indexPath += server.index;
                
                std::ifstream indexFile(indexPath.c_str(), std::ios::binary);
                if (indexFile)
                {
                    std::string content((std::istreambuf_iterator<char>(indexFile)),
                                       std::istreambuf_iterator<char>());
                    std::string response = "HTTP/1.1 200 OK\r\n"
                                         "Content-Type: text/html\r\n"
                                         "Content-Length: " + sizeToString(content.size()) + "\r\n\r\n" + 
                                         content;
                    sendResponse(client_fd, response);
                }
                else
                {
                    std::string errorContent = ErreurDansTaGrosseDaronne(403);
                    std::string response = "HTTP/1.1 403 Forbidden\r\n"
                                         "Content-Type: text/html\r\n"
                                         "Content-Length: " + sizeToString(errorContent.size()) + "\r\n\r\n" +
                                         errorContent;
                    sendResponse(client_fd, response);
                }
            }
        }
        else
        {
            // C'est un fichier normal, le lire et l'envoyer
            std::ifstream file(filePath.c_str(), std::ios::binary);
            if (file)
            {
                std::string content((std::istreambuf_iterator<char>(file)),
                                   std::istreambuf_iterator<char>());
                std::string response = "HTTP/1.1 200 OK\r\n"
                                     "Content-Type: text/html\r\n"
                                     "Content-Length: " + sizeToString(content.size()) + "\r\n\r\n" + 
                                     content;
                sendResponse(client_fd, response);
            }
            else
            {
                std::string errorContent = ErreurDansTaGrosseDaronne(404);
                std::string response = "HTTP/1.1 404 Not Found\r\n"
                                     "Content-Type: text/html\r\n"
                                     "Content-Length: " + sizeToString(errorContent.size()) + "\r\n\r\n" +
                                     errorContent;
                sendResponse(client_fd, response);
            }
        }
    }
    else
    {
        std::string errorContent = ErreurDansTaGrosseDaronne(404);
        std::string response = "HTTP/1.1 404 Not Found\r\n"
                             "Content-Type: text/html\r\n"
                             "Content-Length: " + sizeToString(errorContent.size()) + "\r\n\r\n" +
                             errorContent;
        sendResponse(client_fd, response);
    }
    close(client_fd);
}

void EpollClasse::handlePostRequest(int client_fd, const std::string &request, const std::string &filePath)
{
    // Trouver le début du corps de la requête
    size_t body_pos = request.find("\r\n\r\n");
    if (body_pos == std::string::npos)
    {
        std::string response = "HTTP/1.1 400 Bad Request\r\n"
                             "Content-Type: text/html\r\n\r\n"
                             "<html><body><h1>400 Bad Request</h1></body></html>";
        sendResponse(client_fd, response);
        close(client_fd);
        return;
    }

    std::string body = request.substr(body_pos + 4);
    
    // Créer ou mettre à jour le fichier
    std::ofstream outFile(filePath.c_str());
    if (outFile)
    {
        outFile << body;
        outFile.close();
        
        std::string response = "HTTP/1.1 201 Created\r\n"
                             "Content-Type: text/html\r\n\r\n"
                             "<html><body><h1>201 Created</h1></body></html>";
        sendResponse(client_fd, response);
    }
    else
    {
        std::string response = "HTTP/1.1 403 Forbidden\r\n"
                             "Content-Type: text/html\r\n\r\n"
                             "<html><body><h1>403 Forbidden</h1></body></html>";
        sendResponse(client_fd, response);
    }
    close(client_fd);
}

void EpollClasse::handleDeleteRequest(int client_fd, const std::string &filePath)
{
    if (std::remove(filePath.c_str()) == 0)
    {
        std::string response = "HTTP/1.1 200 OK\r\n"
                             "Content-Type: text/html\r\n\r\n"
                             "<html><body><h1>File deleted successfully</h1></body></html>";
        sendResponse(client_fd, response);
    }
    else
    {
        std::string response = "HTTP/1.1 404 Not Found\r\n"
                             "Content-Type: text/html\r\n\r\n"
                             "<html><body><h1>404 File Not Found</h1></body></html>";
        sendResponse(client_fd, response);
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
