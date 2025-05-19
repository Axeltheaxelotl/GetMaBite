#include"EpollClasse.hpp"
#include"TimeoutManager.hpp"
#include<cerrno>
#include<cstdlib>
#include<fstream>
#include<cstring>
#include<sstream>
#include<sys/stat.h>
#include<sys/wait.h> // Add this include for waitpid
#include<algorithm> // Ensure std::find is available
#include"../utils/Logger.hpp"
#include"../routes/AutoIndex.hpp"
#include"../utils/Utils.hpp"
#include"../http/RequestBufferManager.hpp"
#include"../bonus_cookie/CookieManager.hpp"
#include"../config/ServerNameHandler.hpp"
#include"../cgi/CgiHandler.hpp"

// Fonction utilitaire pour convertir size_t en string (compatible C++98)
static std::string sizeToString(size_t value)
{
	std::ostringstream oss;
	oss << value;
	return oss.str();
}

// Utilitaire pour joindre deux chemins sans double slash
static std::string joinPath(const std::string& left, const std::string& right)
{
	if(left.empty()) return right;
	if(right.empty()) return left;
	if(left[left.size() - 1] == '/' && right[0] == '/')
		return left + right.substr(1);
	if(left[left.size() - 1] != '/' && right[0] != '/')
		return left + "/" + right;
	return left + right;
}

// Utilitaire pour éviter le doublon de dossier (ex: /tests/tests/)
static std::string smartJoinRootAndPath(const std::string& root, const std::string& path)
{
	// netoie les slashes
	std::string cleanRoot = root;
	if(!cleanRoot.empty() && cleanRoot[cleanRoot.size() - 1] == '/')
		cleanRoot = cleanRoot.substr(0, cleanRoot.size() - 1);
	std::string cleanPath = path;
	if(!cleanPath.empty() && cleanPath[0] == '/')
		cleanPath = cleanPath.substr(1);
	// Si le path commence déjà par le nom du dossier root, on ne le rajoute pas
	size_t lastSlash = cleanRoot.find_last_of('/');
	std::string rootDir = (lastSlash != std::string::npos) ? cleanRoot.substr(lastSlash + 1) : cleanRoot;
	if(cleanPath.find(rootDir + "/") == 0)
		return cleanRoot + "/" + cleanPath.substr(rootDir.length() + 1);
	return cleanRoot + "/" + cleanPath;
}

// Constructeur
EpollClasse::EpollClasse() : timeoutManager(10) // Initialize TimeoutManager with a 60-second timeout
{
	_epoll_fd = epoll_create1(0);
	if(_epoll_fd == -1)
	{
		Logger::logMsg(RED, CONSOLE_OUTPUT, "Epoll creation error: %s", strerror(errno));
		exit(EXIT_FAILURE);
	}
	_biggest_fd = 0;
}

// Destructeur
EpollClasse::~EpollClasse()
{
	if(_epoll_fd != -1)
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
	for(std::vector<ServerConfig>::iterator it = _servers.begin(); it != _servers.end(); ++it)
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
	while(true)
	{
		int event_count = epoll_wait(_epoll_fd, _events, MAX_EVENTS, 1000); // Timeout of 1 second
		if(event_count == -1)
		{
			Logger::logMsg(RED, CONSOLE_OUTPUT, "Epoll wait error: %s", strerror(errno));
			exit(EXIT_FAILURE);
		}
		for(int i = 0; i < event_count; ++i)
		{
			int fd = _events[i].data.fd;
			if(_events[i].events & EPOLLIN)
			{
				if(isServerFd(fd))
				{
					acceptConnection(fd);
				}
				else
				{
					handleRequest(fd);
					timeoutManager.updateClientActivity(fd); // Update client activity
				}
			}
			else if(_events[i].events & (EPOLLHUP | EPOLLERR))
			{
				handleError(fd);
			}
		}
		// Check for timed-out clients
		std::vector<int> timedOutClients = timeoutManager.getTimedOutClients();
		for(std::vector<int>::iterator it = timedOutClients.begin(); it != timedOutClients.end(); ++it)
		{
			Logger::logMsg(YELLOW, CONSOLE_OUTPUT, "Client %d timed out. Closing connection.", *it);
			epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, *it, NULL); // Remove client from epoll
			close(*it);
			timeoutManager.removeClient(*it); // Remove client from timeout manager
		}
	}
}

// Ajouter un descripteur à epoll
void EpollClasse::addToEpoll(int fd, epoll_event &event)
{
	if(epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, fd, &event) == -1)
	{
		Logger::logMsg(RED, CONSOLE_OUTPUT, "Epoll add error (FD: %d): %s", fd, strerror(errno));
		exit(EXIT_FAILURE);
	}
	if(fd > _biggest_fd)
	{
		_biggest_fd = fd;
	}
}

// Vérifier si le FD est un serveur
bool EpollClasse::isServerFd(int fd)
{
	for(std::vector<ServerConfig>::iterator it = _servers.begin(); it != _servers.end(); ++it)
	{
		if(fd == it->getFd())
		{
			return true;
		}
	}
	return false;
}

// Trouve un serveur correspondant à un hôte et un port donnés.
int EpollClasse::findMatchingServer(const std::string& host, int port)
{
	for(size_t i = 0; i < _serverConfigs.size(); ++i)
	{
		const Server& server = _serverConfigs[i];
		// Ensure the type of listen_ports matches std::find's requirements
		if(std::find(server.listen_ports.begin(), server.listen_ports.end(), port) != server.listen_ports.end())
		{
			if(ServerNameHandler::isServerNameMatch(server.server_names, host))
			{
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
    Logger::logMsg(YELLOW, CONSOLE_OUTPUT, "\n==================== Nouvelle connexion ====================\n[+] Client connecté depuis %s:%d\n===========================================================", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));
	struct sockaddr_in client_address;
	socklen_t addrlen = sizeof(client_address);
	int client_fd = accept(server_fd, (struct sockaddr *)&client_address, &addrlen);
	if(client_fd == -1)
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
	if(requestedPath == "/")
	{
		// Si index est déjà un chemin absolu, le retourner directement
		if(!server.index.empty() && server.index[0] == '/')
			return server.index;
		// Sinon, concaténer root + index proprement
		return joinPath(server.root, server.index);
	}
	// Pour tout autre chemin, vérifier si une location correspond
	for(std::vector<Location>::const_iterator it = server.locations.begin();
	        it != server.locations.end(); ++it)
	{
		if(requestedPath.find(it->path) == 0)
		{
			// Si un alias est défini, l'utiliser
			if(!it->alias.empty())
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

void EpollClasse::handleCGI(int client_fd, const std::string &cgiPath, const std::string &method, const std::map<std::string, std::string>& cgi_handler, const std::map<std::string, std::string>& cgiParams, const std::string &request)
{
	CgiHandler cgiHandler(client_fd, cgiPath, method, cgi_handler, cgiParams, request);
	std::string response = cgiHandler.executeCgi();
	sendResponse(client_fd, response);
	close(client_fd);
}

// Gérer une requête client
void EpollClasse::handleRequest(int client_fd) {
    char buffer[BUFFER_SIZE];
    int bytes_read = read(client_fd, buffer, BUFFER_SIZE - 1);

    if (bytes_read <= 0) {
        if (bytes_read == 0) {
            Logger::logMsg(YELLOW, CONSOLE_OUTPUT, "Client FD %d closed the connection", client_fd);
        } else {
            Logger::logMsg(RED, CONSOLE_OUTPUT, "Error reading from FD %d: %s", client_fd, strerror(errno));
        }
        close(client_fd);
        timeoutManager.removeClient(client_fd);
        return;
    }

    buffer[bytes_read] = '\0';
    _bufferManager.append(client_fd, std::string(buffer, bytes_read));
    Logger::logMsg(LIGHTMAGENTA, CONSOLE_OUTPUT, "[handleRequest] Buffer for fd %d: %zu bytes", client_fd, _bufferManager.get(client_fd).size());

    if (!_bufferManager.isRequestComplete(client_fd, _serverConfigs[0])) {
        if (_bufferManager.get(client_fd).size() > (size_t)_serverConfigs[0].client_max_body_size) {
            sendErrorResponse(client_fd, 413, _serverConfigs[0]);
            return;
        }

    std::string request = _bufferManager.get(client_fd);
    Logger::logMsg(GREEN, CONSOLE_OUTPUT, "[handleRequest] Full HTTP request received on fd %d:\n%s", client_fd, request.c_str());
    _bufferManager.clear(client_fd);

    // Parser la requête HTTP
    std::string method, path, protocol;
    std::istringstream requestStream(request);
    requestStream >> method >> path >> protocol;

    if (path.empty())
        path = "/";

    // Extraction des headers pour trouver Cookie
    std::string line;
    std::map<std::string, std::string> cookies;
    while (std::getline(requestStream, line) && line != "\r") {
        if (line.find("Cookie:") == 0) {
            std::string cookieHeader = line.substr(7); // après "Cookie:"
            // Trim
            while (!cookieHeader.empty() && (cookieHeader[0] == ' ' || cookieHeader[0] == '\t')) cookieHeader.erase(0, 1);
            cookies = CookieManager::parseCookies(cookieHeader);
        }
    }

    // On utilise le premier serveur pour l'instant (à améliorer pour gérer plusieurs serveurs)
    const Server& server = _serverConfigs[0];

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
            if (matchedLocation->allow_methods[i] == method)
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
            close(client_fd);
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
                        if (server.locations[i].allow_methods[j] == method) {
                            allowed = true;
                            break;
                        }
                    }
                }
            }
        }
        // Si pas de location /, GET seulement autorisé par défaut
        if (!allowed && method == "GET")
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
            close(client_fd);
            return;
        }
    }

    // Utiliser resolvePath pour obtenir le chemin réel
    std::string resolvedPath = resolvePath(server, path);
    Logger::logMsg(GREEN, CONSOLE_OUTPUT, "Resolved path: %s", resolvedPath.c_str());

    // Vérifier si le fichier existe
    struct stat file_stat;
    if (::stat(resolvedPath.c_str(), &file_stat) == 0)
    {
        // Le fichier existe, on le traite selon la méthode
        if (method == "GET" || method == "HEAD")
        {
            // Passer cookies à handleGetRequest (à modifier dans la signature)
            handleGetRequest(client_fd, resolvedPath, server, method == "HEAD", cookies);
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

    close(client_fd);
}

void EpollClasse::sendResponse(int client_fd, const std::string &response)
{
	size_t total_sent = 0;
	while(total_sent < response.size())
	{
		ssize_t sent = send(client_fd, response.c_str() + total_sent, response.size() - total_sent, 0);
		if(sent < 0)
		{
			if(errno == EAGAIN || errno == EWOULDBLOCK)
				continue;
			handleError(client_fd);
			return;
		}
		total_sent += sent;
	}
}

void EpollClasse::handlePostRequest(int client_fd, const std::string &request, const std::string &filePath)
{
	// Trouver le début du corps de la requête
	size_t body_pos = request.find("\r\n\r\n");
	if(body_pos == std::string::npos)
	{
		sendErrorResponse(client_fd, 400, _serverConfigs[0]);
		close(client_fd);
		return;
	}
	std::string body = request.substr(body_pos + 4);
	// Créer ou écraser le fichier (POST doit créer le fichier s'il n'existe pas)
	std::ofstream outFile(filePath.c_str());
	if(outFile)
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
		sendErrorResponse(client_fd, 403, _serverConfigs[0]);
	}
	close(client_fd);
}

void EpollClasse::handleDeleteRequest(int client_fd, const std::string &filePath)
{
	if(remove(filePath.c_str()) == 0)
	{
		std::string response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<html><body><h1>200 OK</h1></body></html>";
		sendResponse(client_fd, response);
	}
	else
	{
		sendErrorResponse(client_fd, 404, _serverConfigs[0]);
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
	if(flags == -1)
	{
		Logger::logMsg(RED, CONSOLE_OUTPUT, "fcntl F_GETFL error: %s", strerror(errno));
		exit(EXIT_FAILURE);
	}
	if(fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
	{
		Logger::logMsg(RED, CONSOLE_OUTPUT, "fcntl F_SETFL error: %s", strerror(errno));
		exit(EXIT_FAILURE);
	}
}

void EpollClasse::handleGetRequest(int client_fd, const std::string &filePath, const Server &server, bool isHead, const std::map<std::string, std::string>& cookies)
{
    struct stat file_stat;
    Logger::logMsg(GREEN, CONSOLE_OUTPUT, "Trying to open file: %s", filePath.c_str());
    // Exemple de gestion d'un cookie de compteur de visites
    int visit_count = 1;
    std::map<std::string, std::string>::const_iterator it = cookies.find("visit_count");
    if (it != cookies.end()) {
        visit_count = atoi(it->second.c_str());
        if (visit_count < 1) visit_count = 1;
        visit_count++;
    }
    std::ostringstream oss;
    oss << visit_count;
    std::string setCookieHeader = CookieManager::createSetCookieHeader("visit_count", oss.str(), 3600, "/");

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
                             << setCookieHeader << "\r\n"
                             << "Content-Type: text/html\r\n"
                             << "Content-Length: " << sizeToString(content.size()) << "\r\n\r\n";
                    if (!isHead)
                        response << content;
                    sendResponse(client_fd, response.str());
                }
                else
                {
                    std::string errorContent = ErreurDansTaGrosseDaronne(403);
                    std::string response = "HTTP/1.1 403 Forbidden\r\n"
                                         + setCookieHeader + "\r\n"
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
                    std::string response = "HTTP/1.1 403 Forbidden\r\n"
                                         + setCookieHeader + "\r\n"
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
                         << setCookieHeader << "\r\n"
                         << "Content-Type: text/html\r\n"
                         << "Content-Length: " << sizeToString(content.size()) << "\r\n\r\n";
                if (!isHead)
                    response << content;
                sendResponse(client_fd, response.str());
            }
            else
            {
                std::string errorContent = ErreurDansTaGrosseDaronne(403);
                std::string response = "HTTP/1.1 403 Forbidden\r\n"
                                     + setCookieHeader + "\r\n"
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
            std::string response = "HTTP/1.1 404 Not Found\r\n"
                                 + setCookieHeader + "\r\n"
                                 + "Content-Type: text/html\r\n"
                                 + "Content-Length: " + sizeToString(errorContent.size()) + "\r\n\r\n"
                                 + errorContent;
            sendResponse(client_fd, response);
        }
    }
    else
    {
        std::string errorContent = ErreurDansTaGrosseDaronne(404);
        std::string response = "HTTP/1.1 404 Not Found\r\n"
                             + setCookieHeader + "\r\n"
                             + "Content-Type: text/html\r\n"
                             + "Content-Length: " + sizeToString(errorContent.size()) + "\r\n\r\n"
                             + errorContent;
        sendResponse(client_fd, response);
    }
    close(client_fd);
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
            // Fichier non trouvé, fallback sur la page par défaut
            body = ErreurDansTaGrosseDaronne(code);
        }
    } else {
        body = ErreurDansTaGrosseDaronne(code);
    }
    std::ostringstream response;
    response << "HTTP/1.1 " << code << " " << status << "\r\n";
    if (code == 405 && !allowHeader.empty()) {
        response << allowHeader << "\r\n";
    }
    response << "Content-Type: " << contentType << "\r\n"
             << "Content-Length: " << body.size() << "\r\n\r\n"
             << body;
    sendResponse(client_fd, response.str());
}
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