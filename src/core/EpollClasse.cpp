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
#include"../routes/RedirectionHandler.hpp"  // Include for return directive handling
#include"../utils/Utils.hpp"
#include"../http/RequestBufferManager.hpp"
#include"../bonus_cookie/CookieManager.hpp"
#include "../bonus_cookie/ParseCookie.hpp"
#include"../config/ServerNameHandler.hpp"
#include"../cgi/CgiHandler.hpp"
#include <map> // Include <map> and <algorithm> for utility implementations

// Fonction utilitaire pour déterminer le type MIME selon l'extension
static std::string getMimeType(const std::string& filename) {
    size_t dot = filename.find_last_of('.');
    if (dot == std::string::npos) {
        return "application/octet-stream";
    }
    std::string ext = filename.substr(dot + 1);
    // C++98-compatible static map initialization
    static std::map<std::string, std::string> mimeMap;
    if (mimeMap.empty()) {
        mimeMap["html"] = "text/html";
        mimeMap["htm"]  = "text/html";
        mimeMap["css"]  = "text/css";
        mimeMap["js"]   = "application/javascript";
        mimeMap["json"] = "application/json";
        mimeMap["png"]  = "image/png";
        mimeMap["jpg"]  = "image/jpeg";
        mimeMap["jpeg"] = "image/jpeg";
        mimeMap["gif"]  = "image/gif";
        mimeMap["ico"]  = "image/x-icon";
        mimeMap["svg"]  = "image/svg+xml";
        mimeMap["txt"]  = "text/plain";
        mimeMap["pdf"]  = "application/pdf";
        mimeMap["mp3"]  = "audio/mpeg";
        mimeMap["mp4"]  = "video/mp4";
    }
    std::map<std::string, std::string>::const_iterator it = mimeMap.find(ext);
    return (it != mimeMap.end()) ? it->second : "application/octet-stream";
}

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
    bool leftEnds = (left[left.size() - 1] == '/');
    bool rightStarts = (!right.empty() && right[0] == '/');
    if (leftEnds && rightStarts) {
        return left + right.substr(1);
    } else if (!leftEnds && !rightStarts) {
        return left + "/" + right;
    }
    return left + right;
}

// Utilitaire pour éviter le doublon de dossier (ex: /tests/tests/)
static std::string smartJoinRootAndPath(const std::string& root, const std::string& path) {
    std::string cleanRoot = root;
    if (!cleanRoot.empty() && cleanRoot[cleanRoot.size() - 1] == '/') {
        cleanRoot.erase(cleanRoot.size() - 1, 1);
    }
    std::string cleanPath = path;
    if (!cleanPath.empty() && cleanPath[0] == '/') {
        cleanPath.erase(0, 1);
    }
    if (cleanRoot.empty()) {
        return std::string("/") + cleanPath;
    }
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
    // Copy server configs and sanitize root paths
    _servers = servers;
    _serverConfigs = serverConfigs;
    // Remove any trailing semicolon from root paths
    for (std::vector<Server>::iterator sit = _serverConfigs.begin(); sit != _serverConfigs.end(); ++sit) {
        if (!sit->root.empty() && sit->root[sit->root.size() - 1] == ';')
            sit->root.erase(sit->root.size() - 1, 1);
    }
    Logger::logMsg(LIGHTMAGENTA, CONSOLE_OUTPUT, "Setting up servers...");
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
        _serverPorts[it->getFd()] = it->getPort();  // Track listening port for each server socket
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
			_bufferManager.remove(*it); // <-- Remove buffer on timeout
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
	// Track which listening port this client is on
	_clientPorts[client_fd] = _serverPorts[server_fd];
	Logger::logMsg(YELLOW, CONSOLE_OUTPUT, "\n==================== Nouvelle connexion ====================\n[+] Client connecté depuis %s:%d\n===========================================================", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));
}

// Résoudre le chemin demandé
std::string EpollClasse::resolvePath(const Server &server, const std::string &requestedPath)
{
	// Si le chemin demandé est la racine, renvoyer le fichier index
	if(requestedPath == "/")
	{
		std::string indexFile = server.index.empty() ? "index.html" : server.index;
		std::string root = server.root;
		if (!root.empty() && root[root.size() - 1] == '/')
			root = root.substr(0, root.size() - 1);
		return root + "/" + indexFile;
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
				std::string alias = it->alias;
				if (!alias.empty() && alias[alias.size() - 1] == '/')
					alias = alias.substr(0, alias.size() - 1);
				// On retire le préfixe de la location
				std::string relative = requestedPath.substr(it->path.length());
				if (!relative.empty() && relative[0] == '/')
					relative = relative.substr(1);
				return alias + (relative.empty() ? "" : "/" + relative);
			}
			// Sinon utiliser le root de la location ou du serveur
			std::string root = !it->root.empty() ? it->root : server.root;
			if (!root.empty() && root[root.size() - 1] == '/')
				root = root.substr(0, root.size() - 1);
			// Si le chemin demandé est exactement la location (ex: /test/), renvoyer index
			if (requestedPath == it->path && !it->index.empty())
				return root + "/" + it->index;
			else if (requestedPath == it->path)
				return root + "/index.html";
			// Correction : retirer le préfixe de la location
			std::string relative = requestedPath.substr(it->path.length());
			if (!relative.empty() && relative[0] == '/')
				relative = relative.substr(1);
			return root + (relative.empty() ? "" : "/" + relative);
		}
	}
	// Si aucune location ne correspond, utiliser le root du serveur
	std::string root = server.root;
	if (!root.empty() && root[root.size() - 1] == '/')
		root = root.substr(0, root.size() - 1);
	std::string relative = requestedPath;
	if (!relative.empty() && relative[0] == '/')
		relative = relative.substr(1);
	// Si root se termine par le même segment que le début de relative, on l'enlève pour éviter duplication (ex: tests/tests)
	size_t slashPos = relative.find('/');
	std::string firstSeg;
	if (slashPos != std::string::npos)
		firstSeg = relative.substr(0, slashPos);
	else
		firstSeg = relative;
	if (!firstSeg.empty() && root.size() >= firstSeg.size() + 1
		&& root.compare(root.size() - firstSeg.size(), firstSeg.size(), firstSeg) == 0)
	{
		// Drop firstSeg and following slash
		if (slashPos != std::string::npos)
			relative = relative.substr(slashPos + 1);
		else
			relative.clear();
	}
	// Use smart join to avoid duplicating folder name (e.g., tests/tests)
	return smartJoinRootAndPath(root, relative);
}

void EpollClasse::handleCGI(int client_fd, const std::string &cgiPath, const std::string &method, const std::map<std::string, std::string>& cgi_handler, const std::map<std::string, std::string>& cgiParams, const std::string &request)
{
	CgiHandler cgiHandler(client_fd, cgiPath, method, cgi_handler, cgiParams, request);
	std::string response = cgiHandler.executeCgi();
	sendResponse(client_fd, response);
}

// Gérer une requête client
void EpollClasse::handleRequest(int client_fd)
{
	char buffer[BUFFER_SIZE];
	int bytes_read = read(client_fd, buffer, BUFFER_SIZE - 1);
	if(bytes_read <= 0)
	{
		if(bytes_read == 0)
		{
			Logger::logMsg(YELLOW, CONSOLE_OUTPUT, "Client FD %d closed the connection", client_fd);
		}
		else
		{
			Logger::logMsg(RED, CONSOLE_OUTPUT, "Error reading from FD %d: %s", client_fd, strerror(errno));
		}
		close(client_fd);
		timeoutManager.removeClient(client_fd);
		_bufferManager.remove(client_fd); // <-- Remove buffer on disconnect
		return;
	}
	buffer[bytes_read] = '\0';
	_bufferManager.append(client_fd, std::string(buffer, bytes_read));
	Logger::logMsg(LIGHTMAGENTA, CONSOLE_OUTPUT, "[handleRequest] Buffer for fd %d: %zu bytes", client_fd, _bufferManager.get(client_fd).size());
	if(!_bufferManager.isRequestComplete(client_fd, _serverConfigs[0]))
	{
		if(_bufferManager.get(client_fd).size() > (size_t)_serverConfigs[0].client_max_body_size)
		{
			Logger::logMsg(RED, CONSOLE_OUTPUT, "[handleRequest] Request too large for fd %d", client_fd);
			Logger::logMsg(RED, CONSOLE_OUTPUT, "[handleRequest] Request size: %zu bytes", _bufferManager.get(client_fd).size());
			Logger::logMsg(RED, CONSOLE_OUTPUT, "[handleRequest] Max body size: %d bytes", _serverConfigs[0].client_max_body_size);
			Logger::logMsg(FUCK_YOU, CONSOLE_OUTPUT, "YOU DUMB FUCK, THE REQUEST SIZE DOES NOT RESET AND GETS ADDED UP AT EVERY REQUEST");
			sendErrorResponse(client_fd, 413, _serverConfigs[0]);
			close(client_fd);
			timeoutManager.removeClient(client_fd);
			_bufferManager.remove(client_fd); // <-- Remove buffer on error
			return;
		}
	}
	std::string request = _bufferManager.get(client_fd);
	Logger::logMsg(GREEN, CONSOLE_OUTPUT, "[handleRequest] Full HTTP request received on fd %d:\n%s", client_fd, request.c_str());
	_bufferManager.clear(client_fd);
	// Parser la requête HTTP
	std::string method, path, protocol;
	std::istringstream requestStream(request);
	requestStream >> method >> path >> protocol;
	if(path.empty())
		path = "/";
	// Extraction des headers pour trouver Host et Cookie
	std::string hostHeader;
    int reqPort = -1;
    // Extraction des headers pour trouver Host et Cookie
    std::string line;
    std::map<std::string, std::string> cookies;
    while(std::getline(requestStream, line) && line != "\r")
    {
        if(line.find("Host:") == 0)
        {
            std::string value = line.substr(5);
            while(!value.empty() && (value[0] == ' ' || value[0] == '\t')) value.erase(0, 1);
            size_t colon = value.find(':');
            if(colon != std::string::npos)
            {
                reqPort = std::atoi(value.substr(colon + 1).c_str());
                value = value.substr(0, colon);
            }
            hostHeader = value;
            continue;
        }
        if(line.find("Cookie:") == 0)
        {
            std::string cookieHeader = line.substr(7); // après "Cookie:"
            // Trim
            while(!cookieHeader.empty() && (cookieHeader[0] == ' ' || cookieHeader[0] == '\t')) cookieHeader.erase(0, 1);
            cookies = parseCookieHeader(cookieHeader);
        }
    }
	// Sélectionner le bon serveur en fonction du Host:port (virtual hosting)
	int listenPort = (reqPort != -1 ? reqPort : _clientPorts[client_fd]);
	int idx = findMatchingServer(hostHeader, listenPort);
	const Server& server = _serverConfigs[(idx >= 0 ? idx : 0)];
	// Trouver la location correspondante (plus long préfixe)
	const Location* matchedLocation = NULL;
	size_t maxMatch = 0;
	for(std::vector<Location>::const_iterator it = server.locations.begin(); it != server.locations.end(); ++it)
	{
		if(path.find(it->path) == 0 && it->path.length() > maxMatch)
		{
			matchedLocation = &(*it);
			maxMatch = it->path.length();
		}
	}
	// Vérification stricte des allow_methods
	std::vector<std::string> allowed_methods;
	if (matchedLocation) {
		allowed_methods = matchedLocation->allow_methods;
		if (allowed_methods.empty())
			allowed_methods = server.allow_methods;
	} else {
		allowed_methods = server.allow_methods;
	}
	if (allowed_methods.empty()) {
		allowed_methods.push_back("GET");
		allowed_methods.push_back("POST");
		allowed_methods.push_back("DELETE");
	}
	bool allowed = false;
	for(size_t i = 0; i < allowed_methods.size(); ++i)
	{
		if(allowed_methods[i] == method)
		{
			allowed = true;
			break;
		}
	}
	if(!allowed)
	{
		// Générer la liste des méthodes autorisées
		std::string allowHeader = "Allow: ";
		for(size_t i = 0; i < allowed_methods.size(); ++i)
		{
			if(i > 0) allowHeader += ", ";
			allowHeader += allowed_methods[i];
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
		timeoutManager.removeClient(client_fd);
		_bufferManager.remove(client_fd); // <-- Remove buffer on 405 error
		return;
	}
	// Handle return directive configured in location
    if (matchedLocation && matchedLocation->return_code != 0)
    {
        int code = matchedLocation->return_code;
        if (code >= 300 && code < 400)
        {
            // Redirection
            std::string url = matchedLocation->return_url;
            std::string response = RedirectionHandler::generateRedirectReponse(code, url);
            sendResponse(client_fd, response);
        }
        else
        {
            // Return error
            sendErrorResponse(client_fd, code, server);
        }
        close(client_fd);
        timeoutManager.removeClient(client_fd);
        _bufferManager.remove(client_fd);
        return;
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
	// Log the resolved path and CGI parameters
	Logger::logMsg(GREEN, CONSOLE_OUTPUT, "Resolved path: %s", resolvedPath.c_str());
	// Vérifier si le fichier existe
	struct stat file_stat;
	if(::stat(resolvedPath.c_str(), &file_stat) == 0)
	{
		size_t cgiPos = resolvedPath.rfind(".cgi.");
		std::string extension;
		if(cgiPos != std::string::npos && cgiPos + 5 < resolvedPath.length())
			extension = resolvedPath.substr(cgiPos + 4); // +4 pour ".cgi"
		std::map<std::string, std::string> cgi_handlers;
		if(!extension.empty())
		{
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
					Logger::logMsg(GREEN, CONSOLE_OUTPUT, "Found CGI handler in location %s: %s",
								   locIt->path.c_str(), cgiIt->second.c_str());
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
				// Convert string to map before passing to handleCGI
				std::map<std::string, std::string> cgiParamMap = parseCGIParams(cgiParam);
				// Pass the body to handleCGI
				handleCGI(client_fd, resolvedPath, method, cgi_handlers, cgiParamMap, request);
			}
			else
			{
				// No handler found for this CGI extension
				sendErrorResponse(client_fd, 501, server);
			}
		}
		else if(method == "GET" || method == "HEAD")
		{
			// Passer cookies à handleGetRequest (à modifier dans la signature)
			handleGetRequest(client_fd, resolvedPath, server, method == "HEAD", cookies);
		}
		else if(method == "POST")
		{
			handlePostRequest(client_fd, request, resolvedPath);
		}
		else if(method == "DELETE")
		{
			handleDeleteRequest(client_fd, resolvedPath);
		}
		else if(method == "PUT")
		{
			std::string uploadPath;
			if(matchedLocation && !matchedLocation->upload_path.empty())
				uploadPath = matchedLocation->upload_path;
			handlePutRequest(client_fd, request, resolvedPath, uploadPath);
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

void EpollClasse::sendResponse(int client_fd, const std::string & response)
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

void EpollClasse::handlePostRequest(int client_fd, const std::string & request, const std::string &filePath)
{
    // Trouver le début du corps de la requête
    size_t body_pos = request.find("\r\n\r\n");
    if(body_pos == std::string::npos)
    {
        sendErrorResponse(client_fd, 400, _serverConfigs[0]);
        close(client_fd);
        return;
    }
    std::string headers = request.substr(0, body_pos);
    std::string body = request.substr(body_pos + 4);
    // Chercher la location correspondante pour upload_path
    std::string uploadPath;
    const Server& server = _serverConfigs[0];
    for(std::vector<Location>::const_iterator it = server.locations.begin(); it != server.locations.end(); ++it)
    {
        if(filePath.find(it->path) == 0 && !it->upload_path.empty())
        {
            uploadPath = it->upload_path;
            break;
        }
    }
    std::string targetPath = filePath;
    if (!uploadPath.empty()) {
        size_t lastSlash = filePath.find_last_of("/");
        std::string filename = (lastSlash != std::string::npos) ? filePath.substr(lastSlash + 1) : filePath;
        targetPath = joinPath(uploadPath, filename);
    }
    // Vérifier si multipart/form-data
    size_t ct_pos = headers.find("Content-Type: multipart/form-data;");
    if (ct_pos != std::string::npos) {
        // Extraire la boundary
        size_t bpos = headers.find("boundary=", ct_pos);
        if (bpos != std::string::npos) {
            std::string boundary = "--" + headers.substr(bpos + 9);
            // Chercher la section du fichier
            size_t part_start = body.find(boundary);
            while (part_start != std::string::npos) {
                part_start += boundary.length();
                if (body.substr(part_start, 2) == "--") break; // fin du multipart
                if (body.substr(part_start, 2) == "\r\n") part_start += 2;
                size_t disp = body.find("Content-Disposition: form-data;", part_start);
                if (disp == std::string::npos) break;
                size_t filename_pos = body.find("filename=\"", disp);
                if (filename_pos == std::string::npos) break;
                size_t header_end = body.find("\r\n\r\n", filename_pos);
                if (header_end == std::string::npos) break;
                size_t file_start = header_end + 4;
                size_t file_end = body.find(boundary, file_start);
                if (file_end == std::string::npos) break;
                // Enlever le CRLF final
                if (file_end > 2 && body[file_end-2] == '\r' && body[file_end-1] == '\n')
                    file_end -= 2;
                std::ofstream outFile(targetPath.c_str(), std::ios::binary);
                if(outFile) {
                    outFile.write(body.c_str() + file_start, file_end - file_start);
                    outFile.close();
                    std::string response = "HTTP/1.1 201 Created\r\nContent-Type: text/html\r\n\r\n<html><body><h1>201 Created</h1></body></html>";
                    sendResponse(client_fd, response);
                } else {
                    sendErrorResponse(client_fd, 403, _serverConfigs[0]);
                }
                close(client_fd);
                return;
            }
        }
    }
    // Sinon, mode texte brut
    std::ofstream outFile(targetPath.c_str());
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

void EpollClasse::handleDeleteRequest(int client_fd, const std::string & filePath)
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

void EpollClasse::handlePutRequest(int client_fd, const std::string &request, const std::string &filePath, const std::string &uploadPath)
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
    std::string targetPath = filePath;
    if (!uploadPath.empty()) {
        // Si upload_path est défini, on écrit dans ce dossier avec le nom du fichier demandé
        size_t lastSlash = filePath.find_last_of("/");
        std::string filename = (lastSlash != std::string::npos) ? filePath.substr(lastSlash + 1) : filePath;
        targetPath = joinPath(uploadPath, filename);
    }
    std::ofstream outFile(targetPath.c_str());
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

// Gérer les erreurs
void EpollClasse::handleError(int fd)
{
	Logger::logMsg(RED, CONSOLE_OUTPUT, "Error on FD: %d", fd);
	close(fd);
	_bufferManager.remove(fd); // <-- Remove buffer on error
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

void EpollClasse::handleGetRequest(int client_fd, const std::string & filePath, const Server & server, bool isHead, const std::map<std::string, std::string>& cookies)
{
	struct stat file_stat;
	Logger::logMsg(GREEN, CONSOLE_OUTPUT, "Trying to open file: %s", filePath.c_str());
	// Exemple de gestion d'un cookie de compteur de visites
	int visit_count = 1;
	std::map<std::string, std::string>::const_iterator it = cookies.find("visit_count");
	if(it != cookies.end())
	{
		visit_count = atoi(it->second.c_str());
		if(visit_count < 1) visit_count = 1;
		visit_count++;
	}
	std::ostringstream oss;
	oss << visit_count;
	std::string setCookieHeader = CookieManager::createSetCookieHeader("visit_count", oss.str(), 3600, "/");
	if(::stat(filePath.c_str(), &file_stat) == 0)
	{
		if(S_ISDIR(file_stat.st_mode))
		{
			std::string indexFile = server.index.empty() ? "index.html" : server.index;
			std::string indexPath = filePath;
			if(indexPath[indexPath.length() - 1] != '/')
				indexPath += "/";
			indexPath += indexFile;
			struct stat index_stat;
			if(::stat(indexPath.c_str(), &index_stat) == 0 && S_ISREG(index_stat.st_mode))
			{
				std::ifstream file(indexPath.c_str(), std::ios::binary);
				if(file)
				{
					std::string content((std::istreambuf_iterator<char>(file)),
					                    std::istreambuf_iterator<char>());
					std::ostringstream response;
					response << "HTTP/1.1 200 OK\r\n"
					         << setCookieHeader << "\r\n"
					         << "Content-Type: text/html\r\n"
					         << "Content-Length: " << sizeToString(content.size()) << "\r\n\r\n";
					if(!isHead)
						response << content;
					sendResponse(client_fd, response.str());
				}
				else
				{
					sendErrorResponse(client_fd, 403, server);
				}
			}
			else
			{
				// Pas de fichier index, vérifier autoindex
				bool autoindex = false;
				for(std::vector<Location>::const_iterator it = server.locations.begin();
				        it != server.locations.end(); ++it)
				{
					if(filePath.find(it->path) == 0)
					{
						autoindex = it->autoindex;
						break;
					}
				}
				if(autoindex)
				{
					std::string content = AutoIndex::generateAutoIndexPage(filePath);
					std::string response = "HTTP/1.1 200 OK\r\n"
					                       "Content-Type: text/html\r\n"
					                       "Content-Length: " + sizeToString(content.size()) + "\r\n\r\n";
					if(!isHead)
						response += content;
					sendResponse(client_fd, response);
				}
				else
				{
					sendErrorResponse(client_fd, 403, server);
				}
			}
		}
		else if(S_ISREG(file_stat.st_mode))
		{
			// C'est un fichier normal, le lire et l'envoyer
			std::ifstream file(filePath.c_str(), std::ios::binary);
			if(file)
			{
				std::string content((std::istreambuf_iterator<char>(file)),
				                    std::istreambuf_iterator<char>());
				std::string mimeType = getMimeType(filePath);
				std::ostringstream response;
				response << "HTTP/1.1 200 OK\r\n"
				         << setCookieHeader << "\r\n"
				         << "Content-Type: " << mimeType << "\r\n"
				         << "Content-Length: " << sizeToString(content.size()) << "\r\n\r\n";
				if(!isHead)
					response << content;
				sendResponse(client_fd, response.str());
			}
			else
			{
				sendErrorResponse(client_fd, 403, server);
			}
		}
		else
		{
			// Ni fichier ni dossier
			sendErrorResponse(client_fd, 404, server);
		}
	}
	else
	{
		sendErrorResponse(client_fd, 404, server);
	}
	close(client_fd);
}

// Envoie une réponse d'erreur HTTP personnalisée si possible
void EpollClasse::sendErrorResponse(int client_fd, int code, const Server & server, const std::string& allowHeader)
{
	std::string body;
	std::string status = StatusCodeString(code);
	std::string contentType = "text/html";
	std::map<int, std::string>::const_iterator it = server.error_pages.find(code);
	bool found = false;
	if(it != server.error_pages.end())
	{
		// Try opening configured error page under server root or relative to CWD
		std::string errorPath = it->second;
		if (!errorPath.empty() && errorPath[0] == '/')
			errorPath = errorPath.substr(1);
		std::string root = server.root;
		if (!root.empty() && root[root.size() - 1] == '/')
			root = root.substr(0, root.size() - 1);
		std::vector<std::string> candidates;
		candidates.push_back(root + "/" + errorPath);
		candidates.push_back(errorPath);
		for (size_t i = 0; i < candidates.size() && !found; ++i) {
			std::ifstream file(candidates[i].c_str());
			if (file.is_open()) {
				std::ostringstream ss;
				ss << file.rdbuf();
				body = ss.str();
				file.close();
				found = true;
			}
		}
	}
	if (!found) {
		// Essayer www/errors/<code>.html
		std::ostringstream path;
		path << "www/errors/" << code << ".html";
		std::ifstream file(path.str().c_str());
		if(file.is_open())
		{
			std::ostringstream ss;
			ss << file.rdbuf();
			body = ss.str();
			file.close();
			found = true;
		}
	}
	if (!found) {
		body = ErreurDansTaGrosseDaronne(code);
	}
	std::ostringstream response;
	response << "HTTP/1.1 " << code << " " << status << "\r\n";
	if(code == 405 && !allowHeader.empty())
	{
		response << allowHeader << "\r\n";
	}
	response << "Content-Type: " << contentType << "\r\n"
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