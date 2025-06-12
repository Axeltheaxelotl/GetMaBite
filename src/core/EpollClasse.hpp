#ifndef EPOLLCLASSE_HPP
#define EPOLLCLASSE_HPP

#include <vector>
#include <string>
#include <sstream>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include "../serverConfig/ServerConfig.hpp"
#include "../config/Server.hpp"
#include "../http/RequestBufferManager.hpp"
#include "../core/TimeoutManager.hpp"

#define MAX_EVENTS 1024
#define BUFFER_SIZE 4096

class EpollClasse {
private:
    int _epoll_fd;
    int _biggest_fd;
    std::vector<ServerConfig> _servers;
    std::vector<Server> _serverConfigs;
    epoll_event _events[MAX_EVENTS];
    RequestBufferManager _bufferManager;
    TimeoutManager timeoutManager;

    void setNonBlocking(int fd);
    bool isServerFd(int fd);
    void addToEpoll(int fd, epoll_event &event);
    void handleError(int fd);
    
    // Méthode pour résoudre les chemins
    std::string resolvePath(const Server &server, const std::string &requestedPath);
    
    // Méthodes de gestion des requêtes HTTP
    void handleGetRequest(int client_fd, const std::string &filePath, const Server &server, bool isHead, const std::map<std::string, std::string>& cookies);
    void handlePostRequest(int client_fd, const std::string &request, const std::string &filePath);
    void handleDeleteRequest(int client_fd, const std::string &filePath);
    void sendResponse(int client_fd, const std::string &response);
    void sendErrorResponse(int client_fd, int code, const Server& server);

    // Finds the matching server based on host and port
    int findMatchingServer(const std::string& host, int port);

    void handleCGI(int client_fd, const std::string &cgiPath, const std::string &method, const std::map<std::string, std::string>& cgi_handler, const std::map<std::string, std::string>& cgiParams, const std::string &body);
    std::map<std::string, std::string> parseCGIParams(const std::string& paramString);


public:
    EpollClasse();
    ~EpollClasse();
    void setupServers(std::vector<ServerConfig> servers, const std::vector<Server> &serverConfigs);
    void serverRun();
    void handleRequest(int client_fd);
    void acceptConnection(int server_fd);
};

#endif