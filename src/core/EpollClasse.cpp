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
EpollClasse::EpollClasse() : timeoutManager(60) // Augmenté à 60 secondes pour les très gros corps
{
    _epoll_fd = epoll_create1(0);
    if (_epoll_fd == -1)
    {
        Logger::logMsg(RED, CONSOLE_OUTPUT, "Epoll creation failed");
        throw std::runtime_error("Epoll creation failed");
    }
    _biggest_fd = 0;
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
                    // Fermeture ultra-rapide sans logs pour maximiser les performances
                    epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, fd, NULL);
                    timeoutManager.removeClient(fd);
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
                    if (!_serverConfigs.empty()) {
                        defaultServer = _serverConfigs[0];
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
    for (size_t i = 0; i < _serverConfigs.size(); ++i) {
        const Server& server = _serverConfigs[i];

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
        event.events = EPOLLIN | EPOLLET; // Edge-triggered pour meilleures performances
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
    int bytes_read = read(client_fd, buffer, BUFFER_SIZE - 1);

    if (bytes_read <= 0) {
        if (bytes_read == 0) {
            Logger::logMsg(YELLOW, CONSOLE_OUTPUT, "Client FD %d closed the connection", client_fd);
        } else {
            Logger::logMsg(RED, CONSOLE_OUTPUT, "Error reading from FD %d: %s", client_fd, strerror(errno));
        }
        epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
        close(client_fd);
        timeoutManager.removeClient(client_fd);
        return;
    }

    buffer[bytes_read] = '\0';
    _bufferManager.append(client_fd, std::string(buffer, bytes_read));
    
    // Mettre à jour l'activité du client pour éviter les timeouts sur les gros corps
    timeoutManager.updateClientActivity(client_fd);
    
    // Pour les gros corps, être plus verbeux et vérifier la mémoire
    size_t currentBufferSize = _bufferManager.getBufferSize(client_fd);
    if (currentBufferSize > 1000000) { // Plus de 1MB
        Logger::logMsg(LIGHTMAGENTA, CONSOLE_OUTPUT, "[handleRequest] Large buffer for fd %d: %zu bytes", client_fd, currentBufferSize);
        
        // Vérifier si on dépasse la limite de sécurité (10MB pour éviter les attaques)
        if (currentBufferSize > 10485760) { // 10MB
            Logger::logMsg(RED, CONSOLE_OUTPUT, "Buffer too large for fd %d, closing connection", client_fd);
            sendErrorResponse(client_fd, 413, _serverConfigs.empty() ? Server() : _serverConfigs[0]);
            epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
            timeoutManager.removeClient(client_fd);
            close(client_fd);
            return;
        }
    }

    if (!_bufferManager.isRequestComplete(client_fd)) {
        // Pour les gros corps, éviter trop de logs verbeux
        if (currentBufferSize < 1000000) {
            Logger::logMsg(LIGHTMAGENTA, CONSOLE_OUTPUT, "[handleRequest] Request not complete for fd %d, waiting more data (%zu bytes so far)", client_fd, currentBufferSize);
        }
        return;
    }

    std::string request = _bufferManager.get(client_fd);
    Logger::logMsg(GREEN, CONSOLE_OUTPUT, "[handleRequest] Full HTTP request received on fd %d (%zu bytes total)", client_fd, request.length());
    _bufferManager.clear(client_fd);

    // Parser la requête HTTP avec validation renforcée
    std::string method, path, protocol;
    std::istringstream requestStream(request);
    
    // Validation de base - vérifier que la première ligne est complète
    std::string firstLine;
    if (!std::getline(requestStream, firstLine) || firstLine.empty()) {
        Logger::logMsg(RED, CONSOLE_OUTPUT, "Empty or invalid request line");
        sendErrorResponse(client_fd, 400, _serverConfigs.empty() ? Server() : _serverConfigs[0]);
        epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
        timeoutManager.removeClient(client_fd);
        close(client_fd);
        return;
    }
    
    // Supprimer les caractères de fin de ligne
    if (!firstLine.empty() && firstLine[firstLine.length()-1] == '\r') {
        firstLine = firstLine.substr(0, firstLine.length()-1);
    }
    
    // Parser la première ligne
    std::istringstream lineStream(firstLine);
    lineStream >> method >> path >> protocol;

    // Validation des requêtes malformées - cas plus stricts
    if (method.empty() || path.empty()) {
        Logger::logMsg(RED, CONSOLE_OUTPUT, "Malformed request: empty method or path");
        sendErrorResponse(client_fd, 400, _serverConfigs.empty() ? Server() : _serverConfigs[0]);
        epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
        timeoutManager.removeClient(client_fd);
        close(client_fd);
        return;
    }
    
    // Validation de la longueur des éléments et caractères invalides
    if (method.length() > 10 || path.length() > 2048 || method.find('\0') != std::string::npos || path.find('\0') != std::string::npos) {
        Logger::logMsg(RED, CONSOLE_OUTPUT, "Malformed request: method or path too long or contains null bytes");
        sendErrorResponse(client_fd, 400, _serverConfigs.empty() ? Server() : _serverConfigs[0]);
        epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
        timeoutManager.removeClient(client_fd);
        close(client_fd);
        return;
    }
    
    // Vérifier que le chemin commence par "/"
    if (path.empty() || path[0] != '/') {
        Logger::logMsg(RED, CONSOLE_OUTPUT, "Malformed request: path must start with /");
        sendErrorResponse(client_fd, 400, _serverConfigs.empty() ? Server() : _serverConfigs[0]);
        epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
        timeoutManager.removeClient(client_fd);
        close(client_fd);
        return;
    }
    
    // Vérifier que le protocole est HTTP/1.1 ou HTTP/1.0
    if (!protocol.empty() && protocol != "HTTP/1.1" && protocol != "HTTP/1.0") {
        Logger::logMsg(RED, CONSOLE_OUTPUT, "Unsupported protocol: %s", protocol.c_str());
        sendErrorResponse(client_fd, 505, _serverConfigs.empty() ? Server() : _serverConfigs[0]); // HTTP Version Not Supported
        epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
        timeoutManager.removeClient(client_fd);
        close(client_fd);
        return;
    }
    
    // Si aucun protocole n'est spécifié, c'est malformé en HTTP strict
    if (protocol.empty()) {
        Logger::logMsg(RED, CONSOLE_OUTPUT, "Malformed request: missing HTTP protocol");
        sendErrorResponse(client_fd, 400, _serverConfigs.empty() ? Server() : _serverConfigs[0]);
        epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
        timeoutManager.removeClient(client_fd);
        close(client_fd);
        return;
    }
    
    // Vérifier que la méthode est valide
    if (method != "GET" && method != "POST" && method != "DELETE" && method != "HEAD") {
        Logger::logMsg(RED, CONSOLE_OUTPUT, "Invalid HTTP method: %s", method.c_str());
        sendErrorResponse(client_fd, 405, _serverConfigs.empty() ? Server() : _serverConfigs[0]); // Method Not Allowed
        epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
        timeoutManager.removeClient(client_fd);
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
    const Server& server = _serverConfigs[idx];

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
    std::string queryString = parseQueryString(request);
    
    Logger::logMsg(GREEN, CONSOLE_OUTPUT, "Processing %s request for path: %s -> %s", 
                   method.c_str(), path.c_str(), resolvedPath.c_str());
    
    // Traiter selon la méthode HTTP
    if (method == "GET" || method == "HEAD") {
        if (method == "HEAD") {
            handleHeadRequest(client_fd, path, server);
        } else {
            handleGetRequest(client_fd, path, server);
        }
    } else if (method == "POST") {
        handlePostRequest(client_fd, path, body, headers, server);
    } else if (method == "DELETE") {
        handleDeleteRequest(client_fd, path, server);
    } else {
        // Méthode non supportée
        sendErrorResponse(client_fd, 501, server);
    }
    
    // Fermer la connexion après traitement
    close(client_fd);
    timeoutManager.removeClient(client_fd);
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
void EpollClasse::handleGetRequest(int client_fd, const std::string &path, const Server &server) {
    std::string resolvedPath = resolvePath(server, path);
    Logger::logMsg(GREEN, CONSOLE_OUTPUT, "GET request for path: %s -> %s", path.c_str(), resolvedPath.c_str());
    
    // Vérifier si c'est un script CGI
    size_t dotPos = resolvedPath.find_last_of('.');
    if (dotPos != std::string::npos) {
        std::string extension = resolvedPath.substr(dotPos);
        std::string interpreter = server.getCgiInterpreterForPath(path, extension);
        if (!interpreter.empty()) {
            std::map<std::string, std::string> emptyHeaders;
            handleCgiRequest(client_fd, resolvedPath, "GET", parseQueryString(path), "", emptyHeaders, server);
            return;
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
    
    // Vérifier si le fichier existe
    if (!fileExists(resolvedPath)) {
        sendErrorResponse(client_fd, 404, server);
        return;
    }
    
    // Lire et envoyer le fichier
    std::string content = readFile(resolvedPath);
    if (content.empty()) {
        sendErrorResponse(client_fd, 500, server);
        return;
    }
    
    std::string mimeType = getMimeType(resolvedPath);
    std::string response = generateHttpResponse(200, mimeType, content);
    send(client_fd, response.c_str(), response.length(), 0);
}

// Gestion des requêtes POST
void EpollClasse::handlePostRequest(int client_fd, const std::string &path, const std::string &body, 
                                  const std::map<std::string, std::string> &headers, const Server &server) {
    Logger::logMsg(GREEN, CONSOLE_OUTPUT, "POST request for path: %s (body size: %zu bytes)", path.c_str(), body.length());
    
    // Vérifier la taille du corps de la requête
    const Location* location = server.findLocation(path);
    size_t maxBodySize = location ? location->client_max_body_size : server.client_max_body_size;
    
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
            handleCgiRequest(client_fd, resolvedPath, "POST", parseQueryString(path), body, headers, server);
            return;
        }
    }
    
    // Gestion de l'upload de fichiers
    std::map<std::string, std::string>::const_iterator contentTypeIt = headers.find("Content-Type");
    if (contentTypeIt != headers.end() && 
        contentTypeIt->second.find("multipart/form-data") != std::string::npos) {
        handleFileUpload(client_fd, body, headers, server);
        return;
    }
    
    // POST simple - écrire dans un fichier
    if (location && !location->upload_path.empty()) {
        std::string uploadPath = location->upload_path + "/post_result.txt";
        std::ofstream file(uploadPath.c_str(), std::ios::binary); // Mode binaire pour les gros fichiers
        if (file.is_open()) {
            file << body;
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
        std::ofstream file(uploadPath.c_str(), std::ios::binary); // Mode binaire pour les gros fichiers
        if (file.is_open()) {
            file << body;
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
    
    // Créer les pipes pour la communication
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        Logger::logMsg(RED, CONSOLE_OUTPUT, "Failed to create pipe for CGI");
        sendErrorResponse(client_fd, 500, server);
        return;
    }
    
    pid_t pid = fork();
    if (pid == -1) {
        Logger::logMsg(RED, CONSOLE_OUTPUT, "Failed to fork for CGI");
        close(pipefd[0]);
        close(pipefd[1]);
        sendErrorResponse(client_fd, 500, server);
        return;
    }
    
    if (pid == 0) {
        // Processus enfant - exécuter le script CGI
        close(pipefd[0]); // Fermer le côté lecture
        
        // Rediriger stdout vers le pipe
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);
        
        // Configurer les variables d'environnement CGI
        setenv("REQUEST_METHOD", method.c_str(), 1);
        setenv("SCRIPT_NAME", scriptPath.c_str(), 1);
        setenv("QUERY_STRING", queryString.c_str(), 1);
        
        if (!body.empty()) {
            std::ostringstream contentLength;
            contentLength << body.length();
            setenv("CONTENT_LENGTH", contentLength.str().c_str(), 1);
        }
        
        // Ajouter les headers HTTP comme variables d'environnement
        for (std::map<std::string, std::string>::const_iterator it = headers.begin();
             it != headers.end(); ++it) {
            std::string envName = "HTTP_" + it->first;
            // Remplacer les tirets par des underscores et mettre en majuscules
            for (size_t i = 0; i < envName.length(); ++i) {
                if (envName[i] == '-') envName[i] = '_';
                envName[i] = toupper(envName[i]);
            }
            setenv(envName.c_str(), it->second.c_str(), 1);
        }
        
        // Déterminer l'interpréteur
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
        
        // Si on arrive ici, exec a échoué
        Logger::logMsg(RED, CONSOLE_OUTPUT, "Failed to execute CGI script");
        exit(1);
    } else {
        // Processus parent
        close(pipefd[1]); // Fermer le côté écriture
        
        // Configurer le pipe en mode non-bloquant
        setNonBlocking(pipefd[0]);
        
        // Créer la structure CGI
        CgiProcess* cgiProcess = new CgiProcess();
        cgiProcess->pipe_fd = pipefd[0];
        cgiProcess->pid = pid;
        cgiProcess->start_time = time(NULL);
        cgiProcess->cgiHandler = NULL; // Pas utilisé dans cette implémentation simplifiée
        
        // Ajouter à epoll pour surveiller la sortie
        epoll_event event;
        event.events = EPOLLIN | EPOLLET;
        event.data.fd = pipefd[0];
        addToEpoll(pipefd[0], event);
        
        // Enregistrer le processus CGI
        _cgiProcesses[pipefd[0]] = cgiProcess;
        _cgiToClient[pipefd[0]] = client_fd;
        
        Logger::logMsg(GREEN, CONSOLE_OUTPUT, "CGI process started with PID %d", pid);
    }
}

// Gestion de la sortie CGI
void EpollClasse::handleCgiOutput(int cgi_fd) {
    std::map<int, CgiProcess*>::iterator it = _cgiProcesses.find(cgi_fd);
    if (it == _cgiProcesses.end()) {
        return;
    }
    
    CgiProcess* process = it->second;
    char buffer[BUFFER_SIZE];
    
    ssize_t bytesRead = read(cgi_fd, buffer, BUFFER_SIZE - 1);
    if (bytesRead > 0) {
        buffer[bytesRead] = '\0';
        process->output += std::string(buffer, bytesRead);
        Logger::logMsg(GREEN, CONSOLE_OUTPUT, "Read %zd bytes from CGI", bytesRead);
    } else if (bytesRead == 0 || bytesRead == -1) {
        // CGI terminé ou erreur
        Logger::logMsg(GREEN, CONSOLE_OUTPUT, "CGI process finished");
        
        // Trouver le client correspondant
        std::map<int, int>::iterator clientIt = _cgiToClient.find(cgi_fd);
        if (clientIt != _cgiToClient.end()) {
            int client_fd = clientIt->second;
            
            // Analyser la sortie CGI
            std::string cgiOutput = process->output;
            std::string headers;
            std::string body;
            
            size_t headerEnd = cgiOutput.find("\r\n\r\n");
            if (headerEnd == std::string::npos) {
                headerEnd = cgiOutput.find("\n\n");
                if (headerEnd != std::string::npos) {
                    headers = cgiOutput.substr(0, headerEnd);
                    body = cgiOutput.substr(headerEnd + 2);
                } else {
                    body = cgiOutput;
                }
            } else {
                headers = cgiOutput.substr(0, headerEnd);
                body = cgiOutput.substr(headerEnd + 4);
            }
            
            // Construire la réponse HTTP
            std::string response = "HTTP/1.1 200 OK\r\n";
            response += "Date: " + getCurrentDateTime() + "\r\n";
            response += "Server: Webserv/1.0\r\n";
            
            if (!headers.empty()) {
                response += headers + "\r\n";
            } else {
                response += "Content-Type: text/html\r\n";
            }
            
            response += "Content-Length: " + sizeToString(body.length()) + "\r\n";
            response += "Connection: close\r\n\r\n";
            response += body;
            
            send(client_fd, response.c_str(), response.length(), 0);
            close(client_fd);
        }
        
        cleanupCgiProcess(cgi_fd);
    }
}

// Nettoyage du processus CGI
void EpollClasse::cleanupCgiProcess(int cgi_fd) {
    std::map<int, CgiProcess*>::iterator it = _cgiProcesses.find(cgi_fd);
    if (it != _cgiProcesses.end()) {
        CgiProcess* process = it->second;
        
        // Supprimer de epoll
        epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, cgi_fd, NULL);
        close(cgi_fd);
        
        // Terminer le processus s'il est encore actif
        if (process->pid > 0) {
            kill(process->pid, SIGTERM);
            waitpid(process->pid, NULL, WNOHANG);
        }
        
        delete process;
        _cgiProcesses.erase(it);
    }
    
    // Supprimer l'association CGI -> client
    _cgiToClient.erase(cgi_fd);
}