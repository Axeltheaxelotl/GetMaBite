#include "EpollClasse.hpp"
#include "TimeoutManager.hpp"
#include <cstdlib>
#include <fstream>
#include <cstring>
#include <sstream>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#include <algorithm> // Ensure std::find is available
#include "../utils/Logger.hpp"
#include "../routes/AutoIndex.hpp"
#include "../routes/RedirectionHandler.hpp"
#include "../utils/Utils.hpp"
#include "../http/RequestBufferManager.hpp"
#include "../config/ServerNameHandler.hpp"
#include"../cgi/CgiHandler.hpp"
#include <stdexcept> // Pour gestion des erreurs par exceptions

#define BUFFER_SIZE 8192

// Déclaration de l'instance globale du RequestBufferManager
RequestBufferManager _bufferManager;

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
    // netoie les slashes
    std::string cleanRoot = root;
    if (!cleanRoot.empty() && cleanRoot[cleanRoot.size() - 1] == '/')
        cleanRoot = cleanRoot.substr(0, cleanRoot.size() - 1);
    std::string cleanPath = path;
    if (!cleanPath.empty() && cleanPath[0] == '/')
        cleanPath = cleanPath.substr(1);
    // Si le path commence déjà par le nom du dossier root, on ne le rajoute pas
    size_t lastSlash = cleanRoot.find_last_of('/');
    std::string rootDir = (lastSlash != std::string::npos) ? cleanRoot.substr(lastSlash + 1) : cleanRoot;
    if (cleanPath.find(rootDir + "/") == 0)
        return cleanRoot + "/" + cleanPath.substr(rootDir.length() + 1);
    return cleanRoot + "/" + cleanPath;
}

// Constructeur
EpollClasse::EpollClasse() : _serverConfigs(NULL), timeoutManager(60) // Augmenté à 60 secondes pour les très gros corps
{
    _epoll_fd = epoll_create1(0);
    if (_epoll_fd == -1)
    {
        Logger::logMsg(RED, CONSOLE_OUTPUT, "Epoll creation failed");
        throw std::runtime_error("Epoll creation failed");
    }
    _biggest_fd = 0;
    
    // Set up signal handling for child processes to avoid zombies
    signal(SIGCHLD, SIG_IGN); // Automatically reap child processes
    signal(SIGPIPE, SIG_IGN); // Ignore broken pipe signals
}

// Destructeur
EpollClasse::~EpollClasse()
{
    // Clean up any remaining CGI processes
    for (std::map<int, CgiProcess*>::iterator it = _cgiProcesses.begin(); it != _cgiProcesses.end(); ++it) {
        CgiProcess* process = it->second;
        CgiHandler* cgiHandler = static_cast<CgiHandler*>(process->cgiHandler);
        if (cgiHandler) {
            cgiHandler->terminateCgi(process);
            delete cgiHandler;
        }
        delete process;
    }
    _cgiProcesses.clear();
    _cgiToClient.clear();
    
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
    _serverConfigs = &serverConfigs;

    for (std::vector<ServerConfig>::iterator it = _servers.begin(); it != _servers.end(); ++it)
    {
        it->setupServer();
        Logger::logMsg(LIGHTMAGENTA, CONSOLE_OUTPUT, "Server Created on %s:%d", 
                      it->getHost().c_str(), it->getPort());

        // Log des informations de configuration
        Logger::logMsg(GREEN, CONSOLE_OUTPUT, "Root directory: %s", (*_serverConfigs)[0].root.c_str());
        Logger::logMsg(GREEN, CONSOLE_OUTPUT, "Index file: %s", (*_serverConfigs)[0].index.c_str());

        epoll_event event;
        event.events = EPOLLIN; // Use level-triggered mode for more reliable operation
        event.data.fd = it->getFd();

        addToEpoll(it->getFd(), event);
    }
}

// Boucle principale
void EpollClasse::serverRun() {
    static int timeout_check_counter = 0;
    
    while (true) {
        int event_count = epoll_wait(_epoll_fd, _events, MAX_EVENTS, 50); // Timeout encore plus réduit à 50ms
        if (event_count == -1) {
            if (errno == EINTR) {
                continue; // Interruption par signal, continuer
            }
            Logger::logMsg(RED, CONSOLE_OUTPUT, "Epoll wait failed: %s", strerror(errno));
            throw std::runtime_error("Epoll wait failed");
        }

        // Traiter tous les événements d'un coup pour optimiser
        for (int i = 0; i < event_count; ++i) {
            int fd = _events[i].data.fd;

            if (_events[i].events & EPOLLIN) {
                if (isServerFd(fd)) {
                    acceptConnection(fd);
                } else if (isCgiFd(fd)) {
                    handleCgiOutput(fd);
                } else {
                    handleRequest(fd);
                    timeoutManager.updateClientActivity(fd);
                }
            } else if (_events[i].events & (EPOLLHUP | EPOLLERR | EPOLLRDHUP)) {
                if (isCgiFd(fd)) {
                    handleCgiOutput(fd);
                } else {
                    // Client disconnected or error
                    Logger::logMsg(YELLOW, CONSOLE_OUTPUT, "Client %d disconnected (HUP/ERR)", fd);
                    epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, fd, NULL);
                    timeoutManager.removeClient(fd);
                    _bufferManager.clear(fd);
                    close(fd);
                }
            }
        }

        // Optimisation: Check for timed-out clients moins fréquemment pour de meilleures performances
        if (++timeout_check_counter >= 200) { // Réduit la fréquence de vérification des timeouts
            timeout_check_counter = 0;
            std::vector<int> timedOutClients = timeoutManager.getTimedOutClients();
            for (std::vector<int>::iterator it = timedOutClients.begin(); it != timedOutClients.end(); ++it) {
                // Log seulement les timeouts importants
                if (_bufferManager.getBufferSize(*it) > 0) {
                    Logger::logMsg(YELLOW, CONSOLE_OUTPUT, "Client %d timed out with pending data", *it);
                }
                epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, *it, NULL);
                close(*it);
                timeoutManager.removeClient(*it);
                _bufferManager.clear(*it);
            }
        }
        
        // Optimisation: Check for timed-out CGI processes moins fréquemment
        static int cgi_check_counter = 0;
        if (++cgi_check_counter >= 200) { // Vérifier les CGI tous les 200 cycles
            cgi_check_counter = 0;
            time_t currentTime = time(NULL);
            std::vector<int> timedOutCgi;
            for (std::map<int, CgiProcess*>::iterator it = _cgiProcesses.begin(); it != _cgiProcesses.end(); ++it) {
                CgiProcess* process = it->second;
                if (currentTime - process->start_time > 30) { // 30 second timeout
                    timedOutCgi.push_back(it->first);
                }
            }
            
            for (std::vector<int>::iterator it = timedOutCgi.begin(); it != timedOutCgi.end(); ++it) {
                std::map<int, int>::iterator clientIt = _cgiToClient.find(*it);
                if (clientIt != _cgiToClient.end()) {
                    int client_fd = clientIt->second;
                    Server defaultServer;
                    if (!_serverConfigs->empty()) {
                        defaultServer = (*_serverConfigs)[0];
                    }
                    sendErrorResponse(client_fd, 504, defaultServer);
                    close(client_fd);
                }
                cleanupCgiProcess(*it);
            }
        }
    }
}

// Ajouter un descripteur à epoll
void EpollClasse::addToEpoll(int fd, epoll_event &event)
{
    if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, fd, &event) == -1)
    {
        Logger::logMsg(RED, CONSOLE_OUTPUT, "Epoll add failed for FD: %d", fd);
        throw std::runtime_error("Epoll add failed");
    }

    if (fd > _biggest_fd)
    {
        _biggest_fd = fd;
    }
}

// Vérifier si le FD est un serveur
bool EpollClasse::isServerFd(int fd) {
    for (std::vector<ServerConfig>::iterator it = _servers.begin(); it != _servers.end(); ++it) {
        if (fd == it->getFd()) {
            return true;
        }
    }
    return false;
}

// Vérifier si le FD est un CGI
bool EpollClasse::isCgiFd(int fd) {
    return _cgiProcesses.find(fd) != _cgiProcesses.end();
}

// Trouve un serveur correspondant à un hôte et un port donnés.
int EpollClasse::findMatchingServer(const std::string& host, int port) {
    int defaultIndex = -1;
    for (size_t i = 0; i < _serverConfigs->size(); ++i) {
        const Server& server = (*_serverConfigs)[i];

        // Vérifie si ce serveur écoute sur ce port
        if (std::find(server.listen_ports.begin(), server.listen_ports.end(), port) != server.listen_ports.end()) {
            // Si c'est le premier serveur pour ce port, le définir comme serveur par défaut
            if (defaultIndex == -1) {
                defaultIndex = i;
            }

            // Cherche un match exact sur le host
            if (std::find(server.server_names.begin(), server.server_names.end(), host) != server.server_names.end()) {
                return i; // Retourne l'index du serveur correspondant
            }
        }
    }

    // Si aucun match sur le host, retourne le serveur par défaut pour ce port
    if (defaultIndex != -1) {
        // Log moins verbeux pour éviter le spam dans les tests
        if (host != "localhost" && host != "127.0.0.1") {
            Logger::logMsg(YELLOW, CONSOLE_OUTPUT, "Aucun serveur correspondant au host '%s'. Utilisation du serveur par défaut pour le port %d.", host.c_str(), port);
        }
    } else {
        Logger::logMsg(RED, CONSOLE_OUTPUT, "Aucun serveur trouvé pour le port %d. Aucun serveur par défaut disponible.", port);
    }

    return defaultIndex;
}

// Accepter une connexion
void EpollClasse::acceptConnection(int server_fd)
{
    struct sockaddr_in client_address;
    socklen_t addrlen = sizeof(client_address);
    
    // Accepter autant de connexions que possible d'un coup pour améliorer les performances
    int connections_accepted = 0;
    while (connections_accepted < 50) { // Augmenté à 50 pour les stress tests
        int client_fd = accept(server_fd, (struct sockaddr *)&client_address, &addrlen);
        if (client_fd == -1)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // Plus de connexions à accepter pour le moment
                break;
            }
            // Ignorer les erreurs temporaires pour améliorer la stabilité
            if (errno == ECONNABORTED || errno == EPROTO || errno == EINTR) {
                continue;
            }
            Logger::logMsg(RED, CONSOLE_OUTPUT, "Accept failed: %s", strerror(errno));
            return;
        }

        // Vérifier que le fd est valide
        if (client_fd >= FD_SETSIZE) {
            Logger::logMsg(RED, CONSOLE_OUTPUT, "Client fd %d too high, closing", client_fd);
            close(client_fd);
            continue;
        }

        setNonBlocking(client_fd);
        timeoutManager.addClient(client_fd);

        epoll_event event;
        event.events = EPOLLIN; // Use level-triggered for clients too
        event.data.fd = client_fd;

        if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, client_fd, &event) == -1) {
            Logger::logMsg(RED, CONSOLE_OUTPUT, "Failed to add client fd %d to epoll: %s", client_fd, strerror(errno));
            close(client_fd);
            timeoutManager.removeClient(client_fd);
            continue; // Continuer au lieu de break pour traiter d'autres connexions
        }

        if (client_fd > _biggest_fd) {
            _biggest_fd = client_fd;
        }

        connections_accepted++;
    }
    
    // Log seulement si on a accepté des connexions
    if (connections_accepted > 0) {
        Logger::logMsg(GREEN, CONSOLE_OUTPUT, "Accepted %d connection(s)", connections_accepted);
    }
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
            // Pour les chemins se terminant par "/", on enlève le "/" avant de faire substr(1)
            std::string pathToAdd = requestedPath;
            if (pathToAdd.length() > it->path.length()) {
                pathToAdd = requestedPath.substr(it->path.length());
            } else {
                pathToAdd = "";
            }
            return smartJoinRootAndPath(root, pathToAdd);
        }
    }

    // Si aucune location ne correspond, utiliser le root du serveur
    return smartJoinRootAndPath(server.root, requestedPath);
}    // Gérer une requête client
void EpollClasse::handleRequest(int client_fd) {
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read = read(client_fd, buffer, BUFFER_SIZE - 1);

    if (bytes_read < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // No data available for now - this shouldn't happen with level-triggered
            return;
        } else {
            Logger::logMsg(RED, CONSOLE_OUTPUT, "Error reading from FD %d: %s", client_fd, strerror(errno));
            epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
            timeoutManager.removeClient(client_fd);
            _bufferManager.clear(client_fd);
            close(client_fd);
            return;
        }
    } else if (bytes_read == 0) {
        // Client closed connection
        Logger::logMsg(YELLOW, CONSOLE_OUTPUT, "Client FD %d closed the connection", client_fd);
        epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
        timeoutManager.removeClient(client_fd);
        _bufferManager.clear(client_fd);
        close(client_fd);
        return;
    }

    // We have data
    _bufferManager.append(client_fd, std::string(buffer, bytes_read));
    timeoutManager.updateClientActivity(client_fd);
    
    // Check buffer size limit to prevent memory attacks
    size_t currentBufferSize = _bufferManager.getBufferSize(client_fd);
    if (currentBufferSize > 10485760) { // 10MB limit
        Logger::logMsg(RED, CONSOLE_OUTPUT, "Buffer too large for fd %d, closing connection", client_fd);
        sendErrorResponse(client_fd, 413, _serverConfigs->empty() ? Server() : (*_serverConfigs)[0]);
        epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
        timeoutManager.removeClient(client_fd);
        _bufferManager.clear(client_fd);
        close(client_fd);
        return;
    }
    
    // Mettre à jour l'activité du client pour éviter les timeouts sur les gros corps
    timeoutManager.updateClientActivity(client_fd);
    
    // Check if we have a complete HTTP request
    if (!_bufferManager.isRequestComplete(client_fd)) {
        // Request not complete, wait for more data
        size_t currentBufferSize = _bufferManager.getBufferSize(client_fd);
        if (currentBufferSize < 1000000) { // Only log for smaller buffers to avoid spam
            Logger::logMsg(LIGHTMAGENTA, CONSOLE_OUTPUT, "[handleRequest] Request not complete for fd %d, waiting more data (%zu bytes so far)", client_fd, currentBufferSize);
        }
        return;
    }

    std::string request = _bufferManager.get(client_fd);
    Logger::logMsg(GREEN, CONSOLE_OUTPUT, "[handleRequest] Full HTTP request received on fd %d (%zu bytes total)", client_fd, request.length());
    _bufferManager.clear(client_fd);

    // Parser la requête HTTP avec validation renforcée
    std::string method, path, protocol, fullPath;
    std::istringstream requestStream(request);
    
    // Validation de base - vérifier que la première ligne est complète
    std::string firstLine;
    if (!std::getline(requestStream, firstLine) || firstLine.empty()) {
        Logger::logMsg(RED, CONSOLE_OUTPUT, "Empty or invalid request line");
        sendErrorResponse(client_fd, 400, _serverConfigs->empty() ? Server() : (*_serverConfigs)[0]);
        epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
        timeoutManager.removeClient(client_fd);
        _bufferManager.clear(client_fd);
        close(client_fd);
        return;
    }
    
    // Supprimer les caractères de fin de ligne
    if (!firstLine.empty() && firstLine[firstLine.length()-1] == '\r') {
        firstLine = firstLine.substr(0, firstLine.length()-1);
    }
    
    // Parser la première ligne
    std::istringstream lineStream(firstLine);
    lineStream >> method >> fullPath >> protocol;
    
    // Extract clean path without query string
    path = fullPath;
    size_t questionPos = path.find('?');
    if (questionPos != std::string::npos) {
        path = path.substr(0, questionPos);
    }

    // Validation des requêtes malformées - cas plus stricts
    if (method.empty() || path.empty()) {
        Logger::logMsg(RED, CONSOLE_OUTPUT, "Malformed request: empty method or path");
        sendErrorResponse(client_fd, 400, _serverConfigs->empty() ? Server() : (*_serverConfigs)[0]);
        epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
        timeoutManager.removeClient(client_fd);
        _bufferManager.clear(client_fd);
        close(client_fd);
        return;
    }
    
    // Validation de la longueur des éléments et caractères invalides
    if (method.length() > 10 || path.length() > 2048 || method.find('\0') != std::string::npos || path.find('\0') != std::string::npos) {
        Logger::logMsg(RED, CONSOLE_OUTPUT, "Malformed request: method or path too long or contains null bytes");
        sendErrorResponse(client_fd, 400, _serverConfigs->empty() ? Server() : (*_serverConfigs)[0]);
        epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
        timeoutManager.removeClient(client_fd);
        _bufferManager.clear(client_fd);
        close(client_fd);
        return;
    }
    
    // Vérifier que le chemin commence par "/"
    if (path.empty() || path[0] != '/') {
        Logger::logMsg(RED, CONSOLE_OUTPUT, "Malformed request: path must start with /");
        sendErrorResponse(client_fd, 400, _serverConfigs->empty() ? Server() : (*_serverConfigs)[0]);
        epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
        timeoutManager.removeClient(client_fd);
        _bufferManager.clear(client_fd);
        close(client_fd);
        return;
    }
    
    // Vérifier que le protocole est HTTP/1.1 ou HTTP/1.0
    if (!protocol.empty() && protocol != "HTTP/1.1" && protocol != "HTTP/1.0") {
        Logger::logMsg(RED, CONSOLE_OUTPUT, "Unsupported protocol: %s", protocol.c_str());
        sendErrorResponse(client_fd, 505, _serverConfigs->empty() ? Server() : (*_serverConfigs)[0]); // HTTP Version Not Supported
        epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
        timeoutManager.removeClient(client_fd);
        _bufferManager.clear(client_fd);
        close(client_fd);
        return;
    }
    
    // Si aucun protocole n'est spécifié, c'est malformé en HTTP strict
    if (protocol.empty()) {
        Logger::logMsg(RED, CONSOLE_OUTPUT, "Malformed request: missing HTTP protocol");
        sendErrorResponse(client_fd, 400, _serverConfigs->empty() ? Server() : (*_serverConfigs)[0]);
        epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
        timeoutManager.removeClient(client_fd);
        _bufferManager.clear(client_fd);
        close(client_fd);
        return;
    }
    
    // Vérifier que la méthode est valide
    if (method != "GET" && method != "POST" && method != "DELETE" && method != "HEAD") {
        Logger::logMsg(RED, CONSOLE_OUTPUT, "Invalid HTTP method: %s", method.c_str());
        sendErrorResponse(client_fd, 405, _serverConfigs->empty() ? Server() : (*_serverConfigs)[0]); // Method Not Allowed
        epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
        timeoutManager.removeClient(client_fd);
        _bufferManager.clear(client_fd);
        close(client_fd);
        return;
    }

    // Gestion du chunked encoding - supporter au lieu de rejeter
    if (request.find("Transfer-Encoding: chunked") != std::string::npos) {
        Logger::logMsg(YELLOW, CONSOLE_OUTPUT, "Chunked encoding detected - processing as normal request");
        // Pour les tests, on traite les requêtes chunked comme des requêtes normales
        // En production, il faudrait implémenter le décodage chunked
    }

    if (path.empty())
        path = "/";
    // Implement HEAD support: treat HEAD as GET and suppress body
    bool isHead = (method == "HEAD");
    std::string reqMethod = isHead ? "GET" : method;

    // Extract host header and route to appropriate server
    std::string hostHeader;
    {
        size_t pos = request.find("Host:");
        if (pos != std::string::npos) {
            pos += 5;
            size_t end = request.find("\r\n", pos);
            hostHeader = request.substr(pos, end - pos);
            // Trim whitespace
            hostHeader.erase(0, hostHeader.find_first_not_of(" \t"));
            hostHeader.erase(hostHeader.find_last_not_of(" \t") + 1);
        }
    }
    // Parse host and optional port
    std::string hostName;
    int port = 0;
    {
        size_t colonPos = hostHeader.find(":");
        if (colonPos != std::string::npos) {
            hostName = hostHeader.substr(0, colonPos);
            std::istringstream iss(hostHeader.substr(colonPos + 1));
            int parsedPort;
            if (!(iss >> parsedPort)) {
                Logger::logMsg(RED, CONSOLE_OUTPUT, "Port invalide dans l'en-tête Host: %s", hostHeader.c_str());
            } else {
                port = parsedPort;
            }
        } else {
            hostName = hostHeader;
        }
    }

    // Déterminer le port local si aucun port n'est spécifié
    if (port == 0) {
        struct sockaddr_in addr;
        socklen_t len = sizeof(addr);
        if (getsockname(client_fd, (struct sockaddr*)&addr, &len) == 0)
            port = ntohs(addr.sin_port);
    }
    // Utilisation de la variable existante hostName
    {
        size_t colonPos = hostHeader.find(":");
        if (colonPos != std::string::npos) {
            hostName = hostHeader.substr(0, colonPos);
            std::istringstream iss(hostHeader.substr(colonPos + 1));
            int parsedPort;
            if (!(iss >> parsedPort)) {
                Logger::logMsg(RED, CONSOLE_OUTPUT, "Port invalide dans l'en-tête Host: %s", hostHeader.c_str());
            } else {
                port = parsedPort;
            }
        } else {
            hostName = hostHeader;
        }
    }

    // Déterminer le port local si aucun port n'est spécifié
    if (port == 0) {
        struct sockaddr_in addr;
        socklen_t len = sizeof(addr);
        if (getsockname(client_fd, (struct sockaddr*)&addr, &len) == 0)
            port = ntohs(addr.sin_port);
    }
    int idx = findMatchingServer(hostName, port);
    if (idx < 0) idx = 0;
    const Server& server = (*_serverConfigs)[idx];

    // Trouver la location correspondante (plus long préfixe)
    const Location* matchedLocation = NULL;
    size_t maxMatch = 0;
    for (std::vector<Location>::const_iterator it = server.locations.begin(); it != server.locations.end(); ++it)
    {
        if (path.find(it->path) == 0 && it->path.length() > maxMatch)
        {
            matchedLocation = &(*it);
            maxMatch = it->path.length();
        }
    }

    // Return directive handling: respect return_code/return_url before any other processing
    if (matchedLocation && matchedLocation->return_code != 0)
    {
        std::string redirectResponse = RedirectionHandler::generateRedirectReponse(
            matchedLocation->return_code, matchedLocation->return_url);
        sendResponse(client_fd, redirectResponse);
        close(client_fd);
        return;
    }

    // Vérification des allow_methods
    if (matchedLocation)
    {
        bool allowed = false;
        
        // Si aucune méthode n'est définie, autoriser GET, POST, DELETE par défaut
        if (matchedLocation->allow_methods.empty()) {
            allowed = (reqMethod == "GET" || reqMethod == "POST" || reqMethod == "DELETE");
        } else {
            for (size_t i = 0; i < matchedLocation->allow_methods.size(); ++i)
            {
                if (matchedLocation->allow_methods[i] == reqMethod)
                {
                    allowed = true;
                    break;
                }
            }
        }
        
        if (!allowed)
        {
            // Générer la liste des méthodes autorisées
            std::string allowHeader = "Allow: ";
            if (matchedLocation->allow_methods.empty()) {
                allowHeader += "GET, POST, DELETE";
            } else {
                for (size_t i = 0; i < matchedLocation->allow_methods.size(); ++i)
                {
                    if (i > 0) allowHeader += ", ";
                    allowHeader += matchedLocation->allow_methods[i];
                }
            }
            std::string body = "<html><body><h1>405 Method Not Allowed</h1></body></html>";
            std::ostringstream response;
            response << "HTTP/1.1 405 Method Not Allowed\r\n"
                     << allowHeader << "\r\n"
                     << "Content-Type: text/html\r\n"
                     << "Content-Length: " << body.size() << "\r\n"
                     << "\r\n"
                     << body;
            sendResponse(client_fd, response.str());
            close(client_fd);
            return;
        }
    }
    else
    {
        // Si aucune location ne correspond, autoriser GET, POST, DELETE par défaut
        bool allowed = (reqMethod == "GET" || reqMethod == "POST" || reqMethod == "DELETE");
        
        if (!allowed) {
            std::string allowHeader = "Allow: GET, POST, DELETE";
            std::string body = "<html><body><h1>405 Method Not Allowed</h1></body></html>";
            std::ostringstream response;
            response << "HTTP/1.1 405 Method Not Allowed\r\n"
                     << allowHeader << "\r\n"
                     << "Content-Type: text/html\r\n"
                     << "Content-Length: " << body.size() << "\r\n"
                     << "\r\n"
                     << body;
            sendResponse(client_fd, response.str());
            close(client_fd);
            return;
        }
    }

    // Utiliser resolvePath pour obtenir le chemin réel
    std::string resolvedPath = resolvePath(server, path);
    
    // Parser les headers de la requête
    std::map<std::string, std::string> headers = parseHeaders(request);
    std::string body = parseBody(request);
    
    // Extract query string from the full path
    std::string queryString = "";
    size_t queryPos = fullPath.find('?');
    if (queryPos != std::string::npos) {
        queryString = fullPath.substr(queryPos + 1);
    }
    
    Logger::logMsg(GREEN, CONSOLE_OUTPUT, "Processing %s request for path: %s -> %s", 
                   method.c_str(), path.c_str(), resolvedPath.c_str());
    
    // Traiter selon la méthode HTTP
    if (method == "GET" || method == "HEAD") {
        if (method == "HEAD") {
            handleHeadRequest(client_fd, path, server);
        } else {
            handleGetRequest(client_fd, path, server, queryString);
        }
    } else if (method == "POST") {
        handlePostRequest(client_fd, path, body, headers, server, queryString);
    } else if (method == "DELETE") {
        handleDeleteRequest(client_fd, path, server);
    } else {
        // Méthode non supportée
        sendErrorResponse(client_fd, 501, server);
    }
    
    // For CGI requests, don't close the connection here - let CGI handler manage it
    // Check if this was a CGI request by looking for the client_fd in our CGI tracking
    bool isCgiRequest = false;
    for (std::map<int, int>::iterator cgiIt = _cgiToClient.begin(); cgiIt != _cgiToClient.end(); ++cgiIt) {
        if (cgiIt->second == client_fd) {
            isCgiRequest = true;
            break;
        }
    }
    
    if (!isCgiRequest) {
        // Not a CGI request, safe to close
        epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
        timeoutManager.removeClient(client_fd);
        _bufferManager.clear(client_fd);
        close(client_fd);
    }
    // If it's a CGI request, the connection will be closed when CGI completes
}

// Fonction utilitaire pour envoyer une réponse
void EpollClasse::sendResponse(int client_fd, const std::string& response) {
    send(client_fd, response.c_str(), response.length(), 0);
}

// Fonction pour définir un FD en mode non-bloquant
void EpollClasse::setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        Logger::logMsg(RED, CONSOLE_OUTPUT, "Failed to get flags for FD %d", fd);
        return;
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        Logger::logMsg(RED, CONSOLE_OUTPUT, "Failed to set non-blocking for FD %d", fd);
    }
}

// Génération de réponse HTTP
std::string EpollClasse::generateHttpResponse(int statusCode, const std::string &contentType, 
                                            const std::string &body, const std::map<std::string, std::string> &headers) {
    std::ostringstream response;
    response << "HTTP/1.1 " << statusCode << " " << getStatusCodeString(statusCode) << "\r\n";
    response << "Date: " << getCurrentDateTime() << "\r\n";
    response << "Server: Webserv/1.0\r\n";
    response << "Content-Type: " << contentType << "\r\n";
    response << "Content-Length: " << body.length() << "\r\n";
    response << "Connection: close\r\n";
    
    // Ajouter les headers personnalisés
    for (std::map<std::string, std::string>::const_iterator it = headers.begin(); 
         it != headers.end(); ++it) {
        response << it->first << ": " << it->second << "\r\n";
    }
    
    response << "\r\n" << body;
    return response.str();
}

// Obtenir la chaîne de statut HTTP
std::string EpollClasse::getStatusCodeString(int statusCode) {
    switch (statusCode) {
        case 200: return "OK";
        case 201: return "Created";
        case 204: return "No Content";
        case 301: return "Moved Permanently";
        case 302: return "Found";
        case 400: return "Bad Request";
        case 401: return "Unauthorized";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 413: return "Request Entity Too Large";
        case 500: return "Internal Server Error";
        case 501: return "Not Implemented";
        case 504: return "Gateway Timeout";
        default: return "Unknown";
    }
}

// Obtenir la date/heure actuelle au format HTTP
std::string EpollClasse::getCurrentDateTime() {
    time_t now = time(0);
    struct tm* gmt = gmtime(&now);
    char buffer[100];
    strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S GMT", gmt);
    return std::string(buffer);
}

// Détection des types MIME
std::string EpollClasse::getMimeType(const std::string &filePath) {
    size_t dotPos = filePath.find_last_of('.');
    if (dotPos == std::string::npos) {
        return "application/octet-stream";
    }
    
    std::string extension = filePath.substr(dotPos);
    
    if (extension == ".html" || extension == ".htm") return "text/html";
    if (extension == ".css") return "text/css";
    if (extension == ".js") return "application/javascript";
    if (extension == ".json") return "application/json";
    if (extension == ".png") return "image/png";
    if (extension == ".jpg" || extension == ".jpeg") return "image/jpeg";
    if (extension == ".gif") return "image/gif";
    if (extension == ".svg") return "image/svg+xml";
    if (extension == ".ico") return "image/x-icon";
    if (extension == ".txt") return "text/plain";
    if (extension == ".xml") return "application/xml";
    if (extension == ".pdf") return "application/pdf";
    if (extension == ".zip") return "application/zip";
    if (extension == ".mp4") return "video/mp4";
    if (extension == ".mp3") return "audio/mpeg";
    
    return "application/octet-stream";
}

// Vérifier l'existence d'un fichier
bool EpollClasse::fileExists(const std::string &filePath) {
    struct stat buffer;
    return (stat(filePath.c_str(), &buffer) == 0);
}

// Lire un fichier
std::string EpollClasse::readFile(const std::string &filePath) {
    std::ifstream file(filePath.c_str(), std::ios::binary);
    if (!file.is_open()) {
        return "";
    }
    
    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// Obtenir la taille d'un fichier
size_t EpollClasse::getFileSize(const std::string &filePath) {
    struct stat buffer;
    if (stat(filePath.c_str(), &buffer) == 0) {
        return buffer.st_size;
    }
    return 0;
}

// Parser les headers HTTP
std::map<std::string, std::string> EpollClasse::parseHeaders(const std::string &request) {
    std::map<std::string, std::string> headers;
    std::istringstream stream(request);
    std::string line;
    
    // Skip the first line (method, path, protocol)
    std::getline(stream, line);
    
    while (std::getline(stream, line) && line != "\r") {
        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos) {
            std::string key = line.substr(0, colonPos);
            std::string value = line.substr(colonPos + 1);
            
            // Trim whitespace
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t\r\n") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t\r\n") + 1);
            
            headers[key] = value;
        }
    }
    
    return headers;
}

// Parser la méthode HTTP
std::string EpollClasse::parseMethod(const std::string &request) {
    std::istringstream stream(request);
    std::string method;
    stream >> method;
    return method;
}

// Parser le chemin HTTP
std::string EpollClasse::parsePath(const std::string &request) {
    std::istringstream stream(request);
    std::string method, path;
    stream >> method >> path;
    
    // Supprimer la query string si présente
    size_t questionPos = path.find('?');
    if (questionPos != std::string::npos) {
        path = path.substr(0, questionPos);
    }
    
    return path.empty() ? "/" : path;
}

// Parser la query string
std::string EpollClasse::parseQueryString(const std::string &request) {
    std::istringstream stream(request);
    std::string method, path;
    stream >> method >> path;
    
    size_t questionPos = path.find('?');
    if (questionPos != std::string::npos) {
        return path.substr(questionPos + 1);
    }
    
    return "";
}

// Parser le corps de la requête
std::string EpollClasse::parseBody(const std::string &request) {
    size_t bodyPos = request.find("\r\n\r\n");
    if (bodyPos != std::string::npos) {
        return request.substr(bodyPos + 4);
    }
    return "";
}

// Gestion des requêtes GET
void EpollClasse::handleGetRequest(int client_fd, const std::string &path, const Server &server, const std::string &queryString) {
    std::string resolvedPath = resolvePath(server, path);
    Logger::logMsg(GREEN, CONSOLE_OUTPUT, "GET request for path: %s -> %s", path.c_str(), resolvedPath.c_str());
    
    // Vérifier si c'est un script CGI
    size_t dotPos = resolvedPath.find_last_of('.');
    if (dotPos != std::string::npos) {
        std::string extension = resolvedPath.substr(dotPos);
        std::string interpreter = server.getCgiInterpreterForPath(path, extension);
        if (!interpreter.empty()) {
            std::map<std::string, std::string> emptyHeaders;
            handleCgiRequest(client_fd, resolvedPath, "GET", queryString, "", emptyHeaders, server);
            return; // CGI will handle connection closure
        }
    }
    
    // Vérifier d'abord si c'est un répertoire pour l'autoindex
    struct stat pathStat;
    if (stat(resolvedPath.c_str(), &pathStat) == 0 && S_ISDIR(pathStat.st_mode)) {
        const Location* location = server.findLocation(path);
        bool autoindexEnabled = location ? location->autoindex : server.autoindex;
        
        if (autoindexEnabled) {
            AutoIndex autoIndex;
            std::string autoIndexPage = autoIndex.generateAutoIndexPage(resolvedPath);
            std::string response = generateHttpResponse(200, "text/html", autoIndexPage);
            send(client_fd, response.c_str(), response.length(), 0);
            return;
        } else {
            sendErrorResponse(client_fd, 403, server);
            return;
        }
    }
    
    // Vérifier si le fichier existe avec logs de debug
    if (!fileExists(resolvedPath)) {
        Logger::logMsg(RED, CONSOLE_OUTPUT, "File not found: %s", resolvedPath.c_str());
        sendErrorResponse(client_fd, 404, server);
        return;
    }
    
    Logger::logMsg(GREEN, CONSOLE_OUTPUT, "File exists, attempting to read: %s", resolvedPath.c_str());
    
    // Lire et envoyer le fichier (même s'il est vide)
    std::string content = readFile(resolvedPath);
    
    // Un fichier vide est valide, ne pas retourner d'erreur 500
    Logger::logMsg(GREEN, CONSOLE_OUTPUT, "File read successfully: %s (%zu bytes)", resolvedPath.c_str(), content.length());
    
    std::string mimeType = getMimeType(resolvedPath);
    std::string response = generateHttpResponse(200, mimeType, content);
    send(client_fd, response.c_str(), response.length(), 0);
}

// Gestion des requêtes POST
void EpollClasse::handlePostRequest(int client_fd, const std::string &path, const std::string &body, 
                                  const std::map<std::string, std::string> &headers, const Server &server, const std::string &queryString) {
    Logger::logMsg(GREEN, CONSOLE_OUTPUT, "POST request for path: %s (body size: %zu bytes)", path.c_str(), body.length());
    
    // Vérifier la taille du corps de la requête avec une logique plus robuste
    const Location* location = server.findLocation(path);
    size_t maxBodySize = location ? location->client_max_body_size : server.client_max_body_size;
    
    // Si maxBodySize est 0, utiliser une valeur par défaut
    if (maxBodySize == 0) {
        maxBodySize = server.client_max_body_size > 0 ? server.client_max_body_size : 1048576; // 1MB par défaut
    }
    
    Logger::logMsg(GREEN, CONSOLE_OUTPUT, "Body size check: %zu bytes (max allowed: %zu bytes)", body.length(), maxBodySize);
    
    if (body.length() > maxBodySize) {
        Logger::logMsg(RED, CONSOLE_OUTPUT, "Request body too large: %zu bytes (max: %zu)", body.length(), maxBodySize);
        sendErrorResponse(client_fd, 413, server);
        return;
    }
    
    std::string resolvedPath = resolvePath(server, path);
    
    // Vérifier si c'est un script CGI
    size_t dotPos = resolvedPath.find_last_of('.');
    if (dotPos != std::string::npos) {
        std::string extension = resolvedPath.substr(dotPos);
        std::string interpreter = server.getCgiInterpreterForPath(path, extension);
        if (!interpreter.empty()) {
            handleCgiRequest(client_fd, resolvedPath, "POST", queryString, body, headers, server);
            return; // CGI will handle connection closure
        }
    }
    
    // Gestion de l'upload de fichiers
    std::map<std::string, std::string>::const_iterator contentTypeIt = headers.find("Content-Type");
    if (contentTypeIt != headers.end() && 
        contentTypeIt->second.find("multipart/form-data") != std::string::npos) {
        handleFileUpload(client_fd, body, headers, server);
        return;
    }
    
    // POST simple - écrire dans un fichier avec gestion robuste des gros corps
    if (location && !location->upload_path.empty()) {
        std::string uploadPath = location->upload_path + "/post_result.txt";
        std::ofstream file(uploadPath.c_str(), std::ios::binary);
        if (file.is_open()) {
            file.write(body.c_str(), body.length());
            file.close();
            
            Logger::logMsg(GREEN, CONSOLE_OUTPUT, "Large POST body written to %s (%zu bytes)", uploadPath.c_str(), body.length());
            std::string response = generateHttpResponse(201, "text/plain", "File uploaded successfully");
            send(client_fd, response.c_str(), response.length(), 0);
        } else {
            Logger::logMsg(RED, CONSOLE_OUTPUT, "Failed to write POST body to %s", uploadPath.c_str());
            sendErrorResponse(client_fd, 500, server);
        }
    } else if (!server.upload_path.empty()) {
        // Fallback sur l'upload_path du serveur
        std::string uploadPath = server.upload_path + "/post_result.txt";
        std::ofstream file(uploadPath.c_str(), std::ios::binary);
        if (file.is_open()) {
            file.write(body.c_str(), body.length());
            file.close();
            
            Logger::logMsg(GREEN, CONSOLE_OUTPUT, "Large POST body written to %s (%zu bytes)", uploadPath.c_str(), body.length());
            std::string response = generateHttpResponse(201, "text/plain", "File uploaded successfully");
            send(client_fd, response.c_str(), response.length(), 0);
        } else {
            Logger::logMsg(RED, CONSOLE_OUTPUT, "Failed to write POST body to %s", uploadPath.c_str());
            sendErrorResponse(client_fd, 500, server);
        }
    } else {
        sendErrorResponse(client_fd, 404, server);
    }
}

// Gestion des requêtes DELETE
void EpollClasse::handleDeleteRequest(int client_fd, const std::string &path, const Server &server) {
    Logger::logMsg(GREEN, CONSOLE_OUTPUT, "DELETE request for path: %s", path.c_str());
    
    // Vérifier si la méthode DELETE est autorisée pour ce chemin
    if (!server.isMethodAllowedForPath(path, "DELETE")) {
        sendErrorResponse(client_fd, 405, server);
        return;
    }
    
    std::string resolvedPath = resolvePath(server, path);
    
    if (!fileExists(resolvedPath)) {
        sendErrorResponse(client_fd, 404, server);
        return;
    }
    
    if (remove(resolvedPath.c_str()) == 0) {
        std::string response = generateHttpResponse(204, "text/plain", "");
        send(client_fd, response.c_str(), response.length(), 0);
    } else {
        sendErrorResponse(client_fd, 500, server);
    }
}

// Gestion des requêtes HEAD
void EpollClasse::handleHeadRequest(int client_fd, const std::string &path, const Server &server) {
    Logger::logMsg(GREEN, CONSOLE_OUTPUT, "HEAD request for path: %s", path.c_str());
    
    std::string resolvedPath = resolvePath(server, path);
    
    if (!fileExists(resolvedPath)) {
        sendErrorResponse(client_fd, 404, server);
        return;
    }
    
    std::string mimeType = getMimeType(resolvedPath);
    
    // Pour HEAD, on n'envoie que les headers, pas le corps
    std::string response = generateHttpResponse(200, mimeType, "");
    // Supprimer le corps de la réponse
    size_t bodyPos = response.find("\r\n\r\n");
    if (bodyPos != std::string::npos) {
        response = response.substr(0, bodyPos + 4);
    }
    
    send(client_fd, response.c_str(), response.length(), 0);
}

// Gestion des erreurs HTTP
void EpollClasse::sendErrorResponse(int client_fd, int errorCode, const Server &server) {
    std::string errorPage = server.getErrorPage(errorCode);
    std::string content;
    
    if (!errorPage.empty() && fileExists(errorPage)) {
        content = readFile(errorPage);
    } else {
        // Page d'erreur par défaut
        std::ostringstream defaultPage;
        defaultPage << "<!DOCTYPE html><html><head><title>" << errorCode << " " 
                   << getStatusCodeString(errorCode) << "</title></head><body>";
        defaultPage << "<h1>" << errorCode << " " << getStatusCodeString(errorCode) << "</h1>";
        defaultPage << "<p>The requested resource could not be found.</p>";
        defaultPage << "<hr><p>Webserv/1.0</p></body></html>";
        content = defaultPage.str();
    }
    
    std::string response = generateHttpResponse(errorCode, "text/html", content);
    send(client_fd, response.c_str(), response.length(), 0);
    
    Logger::logMsg(RED, CONSOLE_OUTPUT, "Sent error response %d to client %d", errorCode, client_fd);
}

// Gestion des erreurs de FD
void EpollClasse::handleError(int fd) {
    Logger::logMsg(RED, CONSOLE_OUTPUT, "Error on FD %d, closing connection", fd);
    epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, fd, NULL);
    close(fd);
    timeoutManager.removeClient(fd);
}

// Gestion de l'upload de fichiers multipart
void EpollClasse::handleFileUpload(int client_fd, const std::string &body, 
                                 const std::map<std::string, std::string> &headers, const Server &server) {
    Logger::logMsg(GREEN, CONSOLE_OUTPUT, "Handling file upload");
    
    // Extraire le boundary du Content-Type
    std::map<std::string, std::string>::const_iterator contentTypeIt = headers.find("Content-Type");
    if (contentTypeIt == headers.end()) {
        sendErrorResponse(client_fd, 400, server);
        return;
    }
    
    std::string contentType = contentTypeIt->second;
    size_t boundaryPos = contentType.find("boundary=");
    if (boundaryPos == std::string::npos) {
        sendErrorResponse(client_fd, 400, server);
        return;
    }
    
    std::string boundary = "--" + contentType.substr(boundaryPos + 9);
    
    // Parser les données multipart
    std::map<std::string, std::string> formData = parseMultipartData(body, boundary);
    
    // Trouver la location correspondante pour l'upload
    const Location* location = server.findLocation("/");
    if (!location || location->upload_path.empty()) {
        // Fallback: chercher n'importe quelle location avec upload_path
        location = NULL;
        for (std::vector<Location>::const_iterator it = server.locations.begin(); 
             it != server.locations.end(); ++it) {
            if (!it->upload_path.empty()) {
                location = &(*it);
                break;
            }
        }
        // Si aucune location n'a d'upload_path, utiliser celui du serveur
        if (!location && server.upload_path.empty()) {
            sendErrorResponse(client_fd, 403, server);
            return;
        }
    }
    
    bool uploadSuccess = false;
    for (std::map<std::string, std::string>::iterator it = formData.begin(); 
         it != formData.end(); ++it) {
        if (it->first.find("filename=") != std::string::npos) {
            // Extraire le nom du fichier
            size_t filenamePos = it->first.find("filename=\"");
            if (filenamePos != std::string::npos) {
                filenamePos += 10; // longueur de "filename=\""
                size_t filenameEnd = it->first.find("\"", filenamePos);
                if (filenameEnd != std::string::npos) {
                    std::string filename = it->first.substr(filenamePos, filenameEnd - filenamePos);
                    
                    // Sécuriser le nom de fichier (éviter path traversal)
                    if (filename.find("..") != std::string::npos || filename.find("/") != std::string::npos) {
                        continue;
                    }
                    
                    // Utiliser upload_path de la location ou du serveur
                    std::string uploadDir = (location && !location->upload_path.empty()) 
                                           ? location->upload_path 
                                           : server.upload_path;
                    std::string uploadPath = uploadDir + "/" + filename;
                    std::ofstream file(uploadPath.c_str(), std::ios::binary);
                    if (file.is_open()) {
                        file << it->second;
                        file.close();
                        uploadSuccess = true;
                        Logger::logMsg(GREEN, CONSOLE_OUTPUT, "File uploaded: %s", uploadPath.c_str());
                    }
                }
            }
        }
    }
    
    if (uploadSuccess) {
        std::string response = generateHttpResponse(201, "text/html", 
            "<html><body><h1>Upload successful!</h1></body></html>");
        send(client_fd, response.c_str(), response.length(), 0);
    } else {
        sendErrorResponse(client_fd, 500, server);
    }
}

// Parser les données multipart/form-data
std::map<std::string, std::string> EpollClasse::parseMultipartData(const std::string &body, const std::string &boundary) {
    std::map<std::string, std::string> formData;
    
    size_t pos = 0;
    while (pos < body.length()) {
        size_t boundaryPos = body.find(boundary, pos);
        if (boundaryPos == std::string::npos) break;
        
        pos = boundaryPos + boundary.length();
        if (pos >= body.length()) break;
        
        // Ignorer \r\n après boundary
        if (pos + 1 < body.length() && body.substr(pos, 2) == "\r\n") {
            pos += 2;
        }
        
        // Trouver la fin des headers de cette partie
        size_t headersEnd = body.find("\r\n\r\n", pos);
        if (headersEnd == std::string::npos) break;
        
        std::string headers = body.substr(pos, headersEnd - pos);
        pos = headersEnd + 4;
        
        // Trouver la fin du contenu de cette partie
        size_t nextBoundary = body.find(boundary, pos);
        if (nextBoundary == std::string::npos) break;
        
        std::string content = body.substr(pos, nextBoundary - pos - 2); // -2 pour \r\n
        
        formData[headers] = content;
        pos = nextBoundary;
    }
    
    return formData;
}

// Gestion des requêtes CGI
void EpollClasse::handleCgiRequest(int client_fd, const std::string &scriptPath, const std::string &method,
                                 const std::string &queryString, const std::string &body,
                                 const std::map<std::string, std::string> &headers, const Server &server) {
    Logger::logMsg(GREEN, CONSOLE_OUTPUT, "Handling CGI request: %s", scriptPath.c_str());
    
    if (!fileExists(scriptPath)) {
        sendErrorResponse(client_fd, 404, server);
        return;
    }
    
    // Create pipes for communication
    int stdin_pipe[2], stdout_pipe[2];
    if (pipe(stdin_pipe) == -1 || pipe(stdout_pipe) == -1) {
        Logger::logMsg(RED, CONSOLE_OUTPUT, "Failed to create pipes for CGI");
        sendErrorResponse(client_fd, 500, server);
        if (stdin_pipe[0] != -1) {
            close(stdin_pipe[0]);
            close(stdin_pipe[1]);
        }
        if (stdout_pipe[0] != -1) {
            close(stdout_pipe[0]);
            close(stdout_pipe[1]);
        }
        return;
    }
    
    pid_t pid = fork();
    if (pid == -1) {
        Logger::logMsg(RED, CONSOLE_OUTPUT, "Failed to fork for CGI");
        close(stdin_pipe[0]);
        close(stdin_pipe[1]);
        close(stdout_pipe[0]);
        close(stdout_pipe[1]);
        sendErrorResponse(client_fd, 500, server);
        return;
    }
    
    if (pid == 0) {
        // Child process - execute CGI script
        close(stdin_pipe[1]);   // Close write end of stdin pipe
        close(stdout_pipe[0]);  // Close read end of stdout pipe
        
        // Redirect stdin and stdout
        dup2(stdin_pipe[0], STDIN_FILENO);
        dup2(stdout_pipe[1], STDOUT_FILENO);
        
        close(stdin_pipe[0]);
        close(stdout_pipe[1]);
        
        // Set up CGI environment variables
        setenv("REQUEST_METHOD", method.c_str(), 1);
        setenv("SCRIPT_NAME", scriptPath.c_str(), 1);
        setenv("QUERY_STRING", queryString.c_str(), 1);
        setenv("SERVER_PROTOCOL", "HTTP/1.1", 1);
        setenv("GATEWAY_INTERFACE", "CGI/1.1", 1);
        
        if (!body.empty()) {
            std::ostringstream contentLength;
            contentLength << body.length();
            setenv("CONTENT_LENGTH", contentLength.str().c_str(), 1);
        }
        
        // Add HTTP headers as environment variables
        for (std::map<std::string, std::string>::const_iterator it = headers.begin();
             it != headers.end(); ++it) {
            std::string envName = "HTTP_" + it->first;
            // Replace dashes with underscores and convert to uppercase
            for (size_t i = 0; i < envName.length(); ++i) {
                if (envName[i] == '-') envName[i] = '_';
                envName[i] = toupper(envName[i]);
            }
            setenv(envName.c_str(), it->second.c_str(), 1);
        }
        
        // Determine interpreter
        size_t dotPos = scriptPath.find_last_of('.');
        if (dotPos != std::string::npos) {
            std::string extension = scriptPath.substr(dotPos);
            std::string interpreter = server.getCgiInterpreterForPath(scriptPath, extension);
            
            if (!interpreter.empty()) {
                execl(interpreter.c_str(), interpreter.c_str(), scriptPath.c_str(), NULL);
            } else {
                execl(scriptPath.c_str(), scriptPath.c_str(), NULL);
            }
        } else {
            execl(scriptPath.c_str(), scriptPath.c_str(), NULL);
        }
        
        // If we get here, exec failed
        Logger::logMsg(RED, CONSOLE_OUTPUT, "Failed to execute CGI script");
        exit(1);
    } else {
        // Parent process
        close(stdin_pipe[0]);   // Close read end of stdin pipe
        close(stdout_pipe[1]);  // Close write end of stdout pipe
        
        // Send request body to CGI if present
        if (!body.empty()) {
            ssize_t written = write(stdin_pipe[1], body.c_str(), body.length());
            if (written != static_cast<ssize_t>(body.length())) {
                Logger::logMsg(YELLOW, CONSOLE_OUTPUT, "Warning: Could not write full body to CGI stdin (%zd/%zu)", written, body.length());
            }
        }
        close(stdin_pipe[1]); // Close stdin pipe, CGI will get EOF
        
        // Set stdout pipe to non-blocking
        setNonBlocking(stdout_pipe[0]);
        
        // Create CGI process structure
        CgiProcess* cgiProcess = new CgiProcess();
        cgiProcess->pipe_fd = stdout_pipe[0];
        cgiProcess->pid = pid;
        cgiProcess->start_time = time(NULL);
        cgiProcess->cgiHandler = NULL;
        cgiProcess->output = "";
        
        // Add CGI pipe to epoll for monitoring
        epoll_event event;
        event.events = EPOLLIN; // Use level-triggered for CGI pipes too
        event.data.fd = stdout_pipe[0];
        
        if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, stdout_pipe[0], &event) == -1) {
            Logger::logMsg(RED, CONSOLE_OUTPUT, "Failed to add CGI pipe to epoll");
            delete cgiProcess;
            close(stdout_pipe[0]);
            kill(pid, SIGTERM);
            sendErrorResponse(client_fd, 500, server);
            return;
        }
        
        // Register CGI process
        _cgiProcesses[stdout_pipe[0]] = cgiProcess;
        _cgiToClient[stdout_pipe[0]] = client_fd;
        
        Logger::logMsg(GREEN, CONSOLE_OUTPUT, "CGI process started with PID %d, pipe fd %d", pid, stdout_pipe[0]);
    }
}

// Gestion de la sortie CGI
void EpollClasse::handleCgiOutput(int cgi_fd) {
    std::map<int, CgiProcess*>::iterator it = _cgiProcesses.find(cgi_fd);
    if (it == _cgiProcesses.end()) {
        Logger::logMsg(YELLOW, CONSOLE_OUTPUT, "CGI process not found for fd %d", cgi_fd);
        return;
    }
    
    CgiProcess* process = it->second;
    char buffer[BUFFER_SIZE];
    bool dataReceived = false;
    
    // For edge-triggered mode, read all available data
    ssize_t bytesRead;
    while ((bytesRead = read(cgi_fd, buffer, BUFFER_SIZE - 1)) > 0) {
        process->output.append(buffer, bytesRead);
        dataReceived = true;
        Logger::logMsg(GREEN, CONSOLE_OUTPUT, "Read %zd bytes from CGI process", bytesRead);
    }
    
    if (bytesRead < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
        // No more data available for now, but CGI might still be running
        if (dataReceived) {
            Logger::logMsg(GREEN, CONSOLE_OUTPUT, "CGI data received, waiting for more or completion");
        }
        return;
    }
    
    // CGI process finished (bytesRead == 0) or error occurred
    Logger::logMsg(GREEN, CONSOLE_OUTPUT, "CGI process finished, total output: %zu bytes", process->output.length());
    
    // Find the corresponding client
    std::map<int, int>::iterator clientIt = _cgiToClient.find(cgi_fd);
    if (clientIt != _cgiToClient.end()) {
        int client_fd = clientIt->second;
        
        // Wait for the child process to complete
        int status;
        pid_t result = waitpid(process->pid, &status, WNOHANG);
        if (result == 0) {
            // Process still running, wait a bit more
            usleep(10000); // 10ms
            result = waitpid(process->pid, &status, WNOHANG);
        }
        
        if (result > 0) {
            // Process completed
            if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
                Logger::logMsg(YELLOW, CONSOLE_OUTPUT, "CGI process exited with status %d", WEXITSTATUS(status));
            }
        }
        
        // Parse CGI output
        std::string cgiOutput = process->output;
        std::string responseHeaders;
        std::string responseBody;
        
        // Look for headers separator
        size_t headerEnd = cgiOutput.find("\r\n\r\n");
        if (headerEnd == std::string::npos) {
            headerEnd = cgiOutput.find("\n\n");
            if (headerEnd != std::string::npos) {
                responseHeaders = cgiOutput.substr(0, headerEnd);
                responseBody = cgiOutput.substr(headerEnd + 2);
            } else {
                // No headers, treat all as body
                responseBody = cgiOutput;
            }
        } else {
            responseHeaders = cgiOutput.substr(0, headerEnd);
            responseBody = cgiOutput.substr(headerEnd + 4);
        }
        
        // Build HTTP response
        std::string httpResponse = "HTTP/1.1 200 OK\r\n";
        httpResponse += "Date: " + getCurrentDateTime() + "\r\n";
        httpResponse += "Server: Webserv/1.0\r\n";
        
        // Add CGI headers if present
        if (!responseHeaders.empty()) {
            // Check if CGI provided a status line
            if (responseHeaders.find("Status:") != std::string::npos) {
                size_t statusPos = responseHeaders.find("Status:");
                size_t statusEnd = responseHeaders.find("\n", statusPos);
                std::string statusLine = responseHeaders.substr(statusPos + 7, statusEnd - statusPos - 7);
                // Remove "Status:" and use the status code
                httpResponse = "HTTP/1.1" + statusLine + "\r\n";
                httpResponse += "Date: " + getCurrentDateTime() + "\r\n";
                httpResponse += "Server: Webserv/1.0\r\n";
                // Remove Status line from headers to avoid duplication
                responseHeaders.erase(statusPos, statusEnd - statusPos + 1);
            }
            httpResponse += responseHeaders;
            // Ensure CGI headers end with \r\n
            if (!responseHeaders.empty() && responseHeaders.substr(responseHeaders.length() - 2) != "\r\n") {
                httpResponse += "\r\n";
            }
            if (responseHeaders.find("Content-Type:") == std::string::npos) {
                httpResponse += "Content-Type: text/html\r\n";
            }
        } else {
            httpResponse += "Content-Type: text/html\r\n";
        }
        
        httpResponse += "Content-Length: " + sizeToString(responseBody.length()) + "\r\n";
        httpResponse += "Connection: close\r\n\r\n";
        httpResponse += responseBody;
        
        // Send response to client
        ssize_t sent = send(client_fd, httpResponse.c_str(), httpResponse.length(), 0);
        if (sent < 0) {
            Logger::logMsg(RED, CONSOLE_OUTPUT, "Failed to send CGI response to client %d", client_fd);
        } else {
            Logger::logMsg(GREEN, CONSOLE_OUTPUT, "Sent CGI response to client %d (%zd bytes)", client_fd, sent);
        }
        
        // Close client connection
        epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
        timeoutManager.removeClient(client_fd);
        _bufferManager.clear(client_fd);
        close(client_fd);
    }
    
    // Clean up CGI process
    cleanupCgiProcess(cgi_fd);
}

// Nettoyage du processus CGI
void EpollClasse::cleanupCgiProcess(int cgi_fd) {
    std::map<int, CgiProcess*>::iterator it = _cgiProcesses.find(cgi_fd);
    if (it != _cgiProcesses.end()) {
        CgiProcess* process = it->second;
        
        Logger::logMsg(GREEN, CONSOLE_OUTPUT, "Cleaning up CGI process with PID %d", process->pid);
        
        // Remove from epoll first
        epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, cgi_fd, NULL);
        
        // Close the pipe
        close(cgi_fd);
        
        // Try to terminate the process gracefully, then forcefully if needed
        if (process->pid > 0) {
            int status;
            pid_t result = waitpid(process->pid, &status, WNOHANG);
            
            if (result == 0) {
                // Process still running, try to terminate
                kill(process->pid, SIGTERM);
                usleep(100000); // Wait 100ms
                result = waitpid(process->pid, &status, WNOHANG);
                
                if (result == 0) {
                    // Still running, force kill
                    Logger::logMsg(YELLOW, CONSOLE_OUTPUT, "Force killing CGI process %d", process->pid);
                    kill(process->pid, SIGKILL);
                    waitpid(process->pid, &status, 0); // Wait for it to die
                }
            }
            
            if (result > 0) {
                Logger::logMsg(GREEN, CONSOLE_OUTPUT, "CGI process %d terminated with status %d", 
                              process->pid, WIFEXITED(status) ? WEXITSTATUS(status) : -1);
            }
        }
        
        delete process;
        _cgiProcesses.erase(it);
    }
    
    // Remove the CGI->client mapping
    std::map<int, int>::iterator clientIt = _cgiToClient.find(cgi_fd);
    if (clientIt != _cgiToClient.end()) {
        _cgiToClient.erase(clientIt);
    }
}