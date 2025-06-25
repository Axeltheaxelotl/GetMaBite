#include "EpollClasse.hpp"
#include "TimeoutManager.hpp"
#include <cerrno>
#include <cstdlib>
#include <fstream>
#include <cstring>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>
#include <algorithm>
#include "../utils/Logger.hpp"
#include "../routes/AutoIndex.hpp"
#include "../utils/Utils.hpp"
#include "../http/RequestBufferManager.hpp"
#include "../config/ServerNameHandler.hpp"
#include"../cgi/CgiHandler.hpp"
#include "../http/MultipartParser.hpp"

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
EpollClasse::EpollClasse() : timeoutManager(10) // Initialize TimeoutManager with a 60-second timeout
{
    _epoll_fd = epoll_create1(0);
    if (_epoll_fd == -1)
    {
        Logger::logMsg(RED, CONSOLE_OUTPUT, "Epoll creation error: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }
    _biggest_fd = 0;
    // Monitor STDIN for exit commands
    epoll_event stdinEvent;
    stdinEvent.events = EPOLLIN;
    stdinEvent.data.fd = STDIN_FILENO;
    addToEpoll(STDIN_FILENO, stdinEvent);
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
void EpollClasse::serverRun() {
    while (true) {
        int event_count = epoll_wait(_epoll_fd, _events, MAX_EVENTS, 1000); // Timeout of 1 second
        if (event_count == -1) {
            Logger::logMsg(RED, CONSOLE_OUTPUT, "Epoll wait error: %s", strerror(errno));
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < event_count; ++i) {
            int fd = _events[i].data.fd;
            // Handle console input for exit
            if (fd == STDIN_FILENO) {
                char inputBuf[16];
                int len = read(STDIN_FILENO, inputBuf, sizeof(inputBuf) - 1);
                if (len > 0) {
                    inputBuf[len] = '\0';
                    std::string cmd(inputBuf);
                    if (cmd.find("exit") != std::string::npos) {
                        Logger::logMsg(GREEN, CONSOLE_OUTPUT, "Exit command received. Shutting down.");
                        return;
                    }
                }
                continue;
            }

            if (_events[i].events & EPOLLIN) {
                if (_cgiHandlers.find(fd) != _cgiHandlers.end()) {
                    processCgiOutput(fd);
                } else if (isServerFd(fd)) {
                    acceptConnection(fd);
                } else {
                    handleRequest(fd);
                    timeoutManager.updateClientActivity(fd); // Update client activity
                }
            } else if (_events[i].events & (EPOLLHUP | EPOLLERR)) {
                if ((_events[i].events & EPOLLHUP) && _cgiHandlers.find(fd) != _cgiHandlers.end()) {
                    processCgiOutput(fd);
                } else {
                    handleError(fd);
                }
            }
        }

        // Check for timed-out clients
        std::vector<int> timedOutClients = timeoutManager.getTimedOutClients();
        for (std::vector<int>::iterator it = timedOutClients.begin(); it != timedOutClients.end(); ++it) {
            Logger::logMsg(YELLOW, CONSOLE_OUTPUT, "Client %d timed out. Closing connection.", *it);
            closeConnection(*it);
        }
        // Exit if exit sent in console
        
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
bool EpollClasse::isServerFd(int fd) {
    for (std::vector<ServerConfig>::iterator it = _servers.begin(); it != _servers.end(); ++it) {
        if (fd == it->getFd()) {
            return true;
        }
    }
    return false;
}

// Trouve un serveur correspondant à un hôte et un port donnés.
int EpollClasse::findMatchingServer(const std::string& host, int port) {
    for (size_t i = 0; i < _serverConfigs.size(); ++i) {
        const Server& server = _serverConfigs[i];
        // Ensure the type of listen_ports matches std::find's requirements
        if (std::find(server.listen_ports.begin(), server.listen_ports.end(), port) != server.listen_ports.end()) {
            if (ServerNameHandler::isServerNameMatch(server.server_names, host)) {
                return i;
            }
        }
    }
    return -1; // No matching server found
}

// Accepter une connexion
void EpollClasse::acceptConnection(int server_fd)
{
    struct sockaddr_in client_address;
    socklen_t addrlen = sizeof(client_address);
    int client_fd = accept(server_fd, (sockaddr*)&client_address, &addrlen);
    if (client_fd == -1)
    {
        Logger::logMsg(RED, CONSOLE_OUTPUT, "Accept error");
        return;
    }
    // Make client socket non-blocking
    setNonBlocking(client_fd);
    epoll_event event;
    event.events = EPOLLIN | EPOLLET; // Lecture et mode edge-triggered
    event.data.fd = client_fd;

    addToEpoll(client_fd, event);
    Logger::logMsg(GREEN, CONSOLE_OUTPUT, "Accepted connection from %s:%d",
                   inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));
    // Add new client to timeout manager
    timeoutManager.addClient(client_fd);
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
void EpollClasse::handleRequest(int client_fd) {
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read = read(client_fd, buffer, BUFFER_SIZE - 1);

    if (bytes_read < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return;
        Logger::logMsg(RED, CONSOLE_OUTPUT, "Error reading from FD %d: %s", client_fd, strerror(errno));
        closeConnection(client_fd);
        return;
    }
    if (bytes_read == 0) {
        Logger::logMsg(YELLOW, CONSOLE_OUTPUT, "Client FD %d closed the connection", client_fd);
        closeConnection(client_fd);
        return;
    }
    buffer[bytes_read] = '\0';
    _bufferManager.append(client_fd, std::string(buffer, bytes_read));
    Logger::logMsg(LIGHTMAGENTA, CONSOLE_OUTPUT, "[handleRequest] Buffer for fd %d: %zu bytes", client_fd, _bufferManager.get(client_fd).size());

    if (!_bufferManager.isRequestComplete(client_fd)) {
        Logger::logMsg(LIGHTMAGENTA, CONSOLE_OUTPUT, "[handleRequest] Request not complete for fd %d, waiting more data", client_fd);
        return;
    }

    std::string request = _bufferManager.get(client_fd);
    Logger::logMsg(GREEN, CONSOLE_OUTPUT, "[handleRequest] Full HTTP request received on fd %d:\n%s", client_fd, request.c_str());

    _bufferManager.clear(client_fd);

    // Parser la requête HTTP
    std::string method, path, protocol;
    std::istringstream requestStream(request);
    requestStream >> method >> path >> protocol;

    // Treat HEAD as GET for permission checks
    bool isHead = (method == "HEAD");
    std::string methodForCheck = isHead ? "GET" : method;

    if (path.empty())
        path = "/";

    // Extract Host header and select server
    std::string host;
    int port = 80;
    {
        size_t headerEnd = request.find("\r\n\r\n");
        size_t hostPos = request.find("\r\nHost:");
        if (hostPos != std::string::npos && hostPos < headerEnd) {
            size_t lineEnd = request.find("\r\n", hostPos + 2);
            std::string hostLine = request.substr(hostPos + 2, lineEnd - (hostPos + 2)); // "Host: value"
            std::string hostValue = hostLine.substr(6); // after "Host: "
            // trim
            while (!hostValue.empty() && (hostValue[0] == ' ' || hostValue[0] == '\t'))
                hostValue.erase(0, 1);
            while (!hostValue.empty() && (hostValue[hostValue.size() - 1] == ' ' || hostValue[hostValue.size() - 1] == '\t'))
                hostValue.erase(hostValue.size() - 1, 1);
            size_t colon = hostValue.find(':');
            if (colon != std::string::npos) {
                host = hostValue.substr(0, colon);
                port = std::atoi(hostValue.substr(colon + 1).c_str());
            } else {
                host = hostValue;
            }
        }
    }
    int serverIndex = findMatchingServer(host, port);
    if (serverIndex == -1) {
        sendErrorResponse(client_fd, 404, _serverConfigs[0]);
        closeConnection(client_fd);
        return;
    }
    const Server& server = _serverConfigs[serverIndex];
    // Enforce client_max_body_size
    size_t headerEnd = request.find("\r\n\r\n");
    size_t bodySize = (headerEnd != std::string::npos) ? request.size() - (headerEnd + 4) : 0;
    if (bodySize > static_cast<size_t>(server.client_max_body_size)) {
        Logger::logMsg(RED, CONSOLE_OUTPUT, "Payload too large: %zu bytes (limit %d)", bodySize, server.client_max_body_size);
        sendErrorResponse(client_fd, 413, server);
        closeConnection(client_fd);
        return;
    }

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

    // Vérification stricte des allow_methods
    if (matchedLocation)
    {
        bool allowed = false;
        for (size_t i = 0; i < matchedLocation->allow_methods.size(); ++i)
        {
            if (matchedLocation->allow_methods[i] == methodForCheck)
            {
                allowed = true;
                break;
            }
        }
        if (!allowed)
        {
            // Générer la liste des méthodes autorisées
            std::string allowHeader = "Allow: ";
            for (size_t i = 0; i < matchedLocation->allow_methods.size(); ++i)
            {
                if (i > 0) allowHeader += ", ";
                allowHeader += matchedLocation->allow_methods[i];
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
            closeConnection(client_fd);
            return;
        }
    }
    else
    {
        // Si aucune location ne matche, vérifier allow_methods du server (ou GET par défaut)
        bool allowed = false;
        if (!server.locations.empty()) {
            // Si le server a une location "/", on peut utiliser ses allow_methods
            for (size_t i = 0; i < server.locations.size(); ++i) {
                if (server.locations[i].path == "/") {
                    for (size_t j = 0; j < server.locations[i].allow_methods.size(); ++j) {
                        if (server.locations[i].allow_methods[j] == methodForCheck) {
                            allowed = true;
                            break;
                        }
                    }
                }
            }
        }

        // Si pas de location /, GET (et HEAD) autorisés par défaut
        if (!allowed && methodForCheck == "GET")
            allowed = true;
        if (!allowed) {
            std::string allowHeader = "Allow: GET";
            std::string body = "<html><body><h1>405 Method Not Allowed</h1></body></html>";
            std::ostringstream response;
            response << "HTTP/1.1 405 Method Not Allowed\r\n"
                     << allowHeader << "\r\n"
                     << "Content-Type: text/html\r\n"
                     << "Content-Length: " << body.size() << "\r\n"
                     << "\r\n"
                     << body;
            sendResponse(client_fd, response.str());
            closeConnection(client_fd);
            return;
        }
    }

    // Utiliser resolvePath pour obtenir le chemin réel
    std::string resolvedPath = resolvePath(server, path);

    // get CGI param
    size_t cgiPos = resolvedPath.rfind("?");
    std::string cgiParam = "";
    if(cgiPos != std::string::npos && cgiPos + 1 < resolvedPath.length())
    {
        cgiParam = resolvedPath.substr(cgiPos + 1); // +1 to skip "?"
        resolvedPath = resolvedPath.substr(0, cgiPos); // Remove the query string from the path
    }
    Logger::logMsg(GREEN, CONSOLE_OUTPUT, "Resolved path: %s", resolvedPath.c_str());
    // Parse CGI parameters
    std::map<std::string, std::string> cgiParamMap = parseCGIParams(cgiParam);

    // Vérifier si le fichier existe
    struct stat file_stat;
    if (::stat(resolvedPath.c_str(), &file_stat) == 0)
    {   
            size_t cgiPos = resolvedPath.rfind(".cgi.");
        if(cgiPos != std::string::npos && cgiPos + 5 < resolvedPath.length())
        {
            // Get the extension part after ".cgi."
            std::string extension = resolvedPath.substr(cgiPos + 4); // +4 to skip ".cgi"
            // Find the matching location and check if the extension is supported
            Logger::logMsg(GREEN, CONSOLE_OUTPUT, "Extension: %s", extension.c_str());
            std::map<std::string, std::string> cgi_handlers;
            // First check the matched location (if we have one)
            if(matchedLocation)
            {
                std::map<std::string, std::string>::const_iterator cgiIt =
                    matchedLocation->cgi_extensions.find(extension);
                if(cgiIt != matchedLocation->cgi_extensions.end())
                {
                    cgi_handlers[extension + "_location"] = cgiIt->second;
                    Logger::logMsg(GREEN, CONSOLE_OUTPUT, "Found CGI handler in matched location: %s", cgiIt->second.c_str());
                }
            }
            // Check all server-wide locations (collect all matches)
            for(std::vector<Location>::const_iterator locIt = server.locations.begin();
                    locIt != server.locations.end(); ++locIt)
            {
                std::map<std::string, std::string>::const_iterator cgiIt =
                    locIt->cgi_extensions.find(extension);
                if(cgiIt != locIt->cgi_extensions.end())
                {
                    // Use a unique key by appending the location path
                    cgi_handlers[extension + "_" + locIt->path] = cgiIt->second;
                    Logger::logMsg(GREEN, CONSOLE_OUTPUT, "Found CGI handler in location %s: %s",
                                   locIt->path.c_str(), cgiIt->second.c_str());
                }
            }
            // Check server-wide CGI extensions
            std::map<std::string, std::string>::const_iterator serverCgiIt =
                server.cgi_extensions.find(extension);
            if(serverCgiIt != server.cgi_extensions.end())
            {
                cgi_handlers[extension] = serverCgiIt->second;
                Logger::logMsg(GREEN, CONSOLE_OUTPUT, "Found server-wide CGI handler: %s", serverCgiIt->second.c_str());
            }
            if(!cgi_handlers.empty())
            {
                handleCGI(client_fd, resolvedPath, method, cgi_handlers, cgiParamMap, request);
                return;
            }
            else
            {
                // No handler found for this CGI extension
                sendErrorResponse(client_fd, 501, server);
            }
        }
        // Le fichier existe, on le traite selon la méthode
        else if (method == "GET" || method == "HEAD")
        {
            handleGetRequest(client_fd, resolvedPath, server, method == "HEAD");
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
            // Méthode non supportée (ne devrait plus arriver)
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
        sendErrorResponse(client_fd, 404, server);
    }

    closeConnection(client_fd);
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

void EpollClasse::handleCGI(int client_fd, const std::string &cgiPath, const std::string &method, const std::map<std::string, std::string>& cgi_handler, const std::map<std::string, std::string>& cgiParams, const std::string &request)
{
	CgiHandler* handler = new CgiHandler(client_fd, cgiPath, method, cgi_handler, cgiParams, request);
    handler->forkProcess();
    int outFd = handler->getOutputFd();
    setNonBlocking(outFd);
    epoll_event ev;
    ev.events = EPOLLIN | EPOLLET | EPOLLHUP;
    ev.data.fd = outFd;
    addToEpoll(outFd, ev);
    _cgiHandlers[outFd] = handler;
    _cgiBuffers[outFd] = "";
}

// Handle asynchronous CGI output
void EpollClasse::processCgiOutput(int fd)
{
    char buffer[BUFFER_SIZE];
    ssize_t n;
    while ((n = read(fd, buffer, BUFFER_SIZE)) > 0) {
        _cgiBuffers[fd].append(buffer, n);
    }
    if (n == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
        return;
    }
    CgiHandler* handler = _cgiHandlers[fd];
    int client_fd = handler->getClientFd();
    sendResponse(client_fd, _cgiBuffers[fd]);
    // cleanup CGI output FD
    epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, fd, NULL);
    close(fd);
    delete handler;
    _cgiHandlers.erase(fd);
    _cgiBuffers.erase(fd);
    closeConnection(client_fd);
}

void EpollClasse::handlePostRequest(int client_fd, const std::string &request, const std::string &filePath)
{
    // Séparer headers et body
    size_t header_end = request.find("\r\n\r\n");
    std::string header = request.substr(0, header_end);
    std::string body = (header_end != std::string::npos) ? request.substr(header_end + 4) : "";

    // Extraire Content-Type
    std::string contentType;
    std::istringstream hs(header);
    std::string line;
    while (std::getline(hs, line) && !line.empty()) {
        if (line.find("Content-Type:") == 0) {
            contentType = line.substr(14);
            break;
        }
    }

    const std::string multipart = "multipart/form-data; boundary=";
    if (contentType.find(multipart) == 0) {
        // Upload multipart
        std::string boundary = contentType.substr(multipart.size());
        std::vector<MultipartPart> parts = MultipartParser::parse(body, boundary);
        for (size_t i = 0; i < parts.size(); ++i) {
            const MultipartPart &p = parts[i];
            if (!p.filename.empty()) {
                std::string dst = filePath + "/" + p.filename;
                std::ofstream ofs(dst.c_str(), std::ios::binary);
                ofs.write(p.data.data(), p.data.size());
            }
        }
        std::ostringstream resp;
        resp << "HTTP/1.1 201 Created\r\n"
             << "Content-Length: 0\r\n\r\n";
        sendResponse(client_fd, resp.str());
    } else {
        // Upload simple
        std::ofstream ofs(filePath.c_str(), std::ios::binary);
        ofs.write(body.data(), body.size());
        std::ostringstream resp;
        resp << "HTTP/1.1 201 Created\r\n"
             << "Content-Length: " << body.size() << "\r\n\r\n"
             << body;
        sendResponse(client_fd, resp.str());
    }
    closeConnection(client_fd);
}

void EpollClasse::handleDeleteRequest(int client_fd, const std::string &filePath)
{
    if (remove(filePath.c_str()) == 0)
    {
        std::string response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<html><body><h1>200 OK</h1></body></html>";
        sendResponse(client_fd, response);
    }
    else
    {
        sendErrorResponse(client_fd, 404, _serverConfigs[0]);
    }
    closeConnection(client_fd);
}

// Centralized client close: remove from epoll and timeout, then close
void EpollClasse::closeConnection(int fd)
{
    epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, fd, NULL);
    timeoutManager.removeClient(fd);
    close(fd);
}

// Gérer les erreurs
void EpollClasse::handleError_impl(int fd, const char* file, int line, const char* caller)
{
    Logger::logMsg(RED, CONSOLE_OUTPUT, "Error on FD %d in %s:%d called by %s: %s", 
                   fd, file, line, caller, strerror(errno));
    closeConnection(fd);
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

void EpollClasse::handleGetRequest(int client_fd, const std::string &filePath, const Server &server, bool isHead)
{
    struct stat file_stat;
    Logger::logMsg(GREEN, CONSOLE_OUTPUT, "Trying to open file: %s", filePath.c_str());

    if (::stat(filePath.c_str(), &file_stat) == 0)
    {
        if (S_ISDIR(file_stat.st_mode))
        {
            std::string indexFile = server.index.empty() ? "index.html" : server.index;
            std::string indexPath = filePath;
            if (indexPath[indexPath.length() - 1] != '/')
                indexPath += "/";
            indexPath += indexFile;
            struct stat index_stat;
            if (::stat(indexPath.c_str(), &index_stat) == 0 && S_ISREG(index_stat.st_mode))
            {
                std::ifstream file(indexPath.c_str(), std::ios::binary);
                if (file)
                {
                    std::string content((std::istreambuf_iterator<char>(file)),
                                       std::istreambuf_iterator<char>());
                    std::ostringstream response;
                    response << "HTTP/1.1 200 OK\r\n"
                              << "Content-Type: text/html\r\n"
                              << "Content-Length: " << sizeToString(content.size()) << "\r\n\r\n";
                    if (!isHead)
                        response << content;
                    sendResponse(client_fd, response.str());
                }
                else
                {
                    std::string errorContent = ErreurDansTaGrosseDaronne(403);
                    std::string response = std::string("HTTP/1.1 403 Forbidden\r\n")
                                         + "Content-Type: text/html\r\n"
                                         + "Content-Length: " + sizeToString(errorContent.size()) + "\r\n\r\n"
                                         + errorContent;
                    sendResponse(client_fd, response);
                }
            }
            else
            {
                // Pas de fichier index, vérifier autoindex
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
                                         "Content-Length: " + sizeToString(content.size()) + "\r\n\r\n";
                    if (!isHead)
                        response += content;
                    sendResponse(client_fd, response);
                }
                else
                {
                    std::string errorContent = ErreurDansTaGrosseDaronne(403);
                    std::string response = std::string("HTTP/1.1 403 Forbidden\r\n")
                                         + "Content-Type: text/html\r\n"
                                         + "Content-Length: " + sizeToString(errorContent.size()) + "\r\n\r\n"
                                         + errorContent;
                    sendResponse(client_fd, response);
                }
            }
        }
        else if (S_ISREG(file_stat.st_mode))
        {
            // C'est un fichier normal, le lire et l'envoyer
            std::ifstream file(filePath.c_str(), std::ios::binary);
            if (file)
            {
                std::string content((std::istreambuf_iterator<char>(file)),
                                   std::istreambuf_iterator<char>());
                std::ostringstream response;
                response << "HTTP/1.1 200 OK\r\n"
                         << "Content-Type: text/html\r\n"
                         << "Content-Length: " << sizeToString(content.size()) << "\r\n\r\n";
                if (!isHead)
                    response << content;
                sendResponse(client_fd, response.str());
            }
            else
            {
                std::string errorContent = ErreurDansTaGrosseDaronne(403);
                std::string response = std::string("HTTP/1.1 403 Forbidden\r\n")
                                     + "Content-Type: text/html\r\n"
                                     + "Content-Length: " + sizeToString(errorContent.size()) + "\r\n\r\n"
                                     + errorContent;
                sendResponse(client_fd, response);
            }
        }
        else
        {
            // Ni fichier ni dossier
            std::string errorContent = ErreurDansTaGrosseDaronne(404);
            std::string response = std::string("HTTP/1.1 404 Not Found\r\n")
                                 + "Content-Type: text/html\r\n"
                                 + "Content-Length: " + sizeToString(errorContent.size()) + "\r\n\r\n"
                                 + errorContent;
            sendResponse(client_fd, response);
        }
    }
    else
    {
        std::string errorContent = ErreurDansTaGrosseDaronne(404);
        std::string response = std::string("HTTP/1.1 404 Not Found\r\n")
                             + "Content-Type: text/html\r\n"
                             + "Content-Length: " + sizeToString(errorContent.size()) + "\r\n\r\n"
                             + errorContent;
        sendResponse(client_fd, response);
    }
}

// Envoie une réponse d'erreur HTTP personnalisée si possible
void EpollClasse::sendErrorResponse(int client_fd, int code, const Server& server) {
    std::string body;
    std::string status = StatusCodeString(code);
    std::string contentType = "text/html";
    std::string customPath;
    std::map<int, std::string>::const_iterator it = server.error_pages.find(code);
    if (it != server.error_pages.end()) {
        // On tente de lire le fichier d'erreur personnalisé
        std::ifstream file(it->second.c_str());
        if (file.is_open()) {
            std::ostringstream ss;
            ss << file.rdbuf();
            body = ss.str();
            file.close();
        } else {
            // Fallback: try default error page in www/errors/<code>.html
            std::ostringstream defaultPath;
            defaultPath << "www/errors/" << code << ".html";
            std::ifstream defaultFile(defaultPath.str().c_str());
            if (defaultFile.is_open()) {
                std::ostringstream ssDefault;
                ssDefault << defaultFile.rdbuf();
                body = ssDefault.str();
                defaultFile.close();
            } else {
                body = ErreurDansTaGrosseDaronne(code);
            }
        }
    } else {
        // No custom error page, try default directory
        std::ostringstream defaultPath;
        defaultPath << "www/errors/" << code << ".html";
        std::ifstream defaultFile(defaultPath.str().c_str());
        if (defaultFile.is_open()) {
            std::ostringstream ssDefault;
            ssDefault << defaultFile.rdbuf();
            body = ssDefault.str();
            defaultFile.close();
        } else {
            body = ErreurDansTaGrosseDaronne(code);
        }
    }
    std::ostringstream response;
    response << "HTTP/1.1 " << code << " " << status << "\r\n"
             << "Content-Type: " << contentType << "\r\n"
             << "Content-Length: " << body.size() << "\r\n\r\n"
             << body;
    sendResponse(client_fd, response.str());
}

// Utility function to parse CGI query parameters
std::map<std::string, std::string> EpollClasse::parseCGIParams(const std::string& paramString)
{
	std::map<std::string, std::string> params;
	std::istringstream stream(paramString);
	std::string pair;
	while(std::getline(stream, pair, '&'))
	{
		size_t pos = pair.find('=');
		if(pos != std::string::npos)
		{
			std::string key = pair.substr(0, pos);
			std::string value = pair.substr(pos + 1);
			params[key] = value;
		}
		else if(!pair.empty())
		{
			// If there's no '=', use the parameter as key with empty value
			params[pair] = "";
		}
	}
	return params;
}