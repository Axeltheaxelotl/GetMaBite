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

#define MAX_EVENTS 1024
#define BUFFER_SIZE 4096

class EpollClasse {
private:
    int _epoll_fd;
    int _biggest_fd;
    std::vector<ServerConfig> _servers;
    std::vector<Server> _serverConfigs;  // C'est la config qui contient root, index, etc.
    epoll_event _events[MAX_EVENTS];

    void setNonBlocking(int fd);
    bool isServerFd(int fd);
    void addToEpoll(int fd, epoll_event &event);
    void handleError(int fd);
    
    // Méthode pour résoudre les chemins
    std::string resolvePath(const Server &server, const std::string &requestedPath);
    
    // Méthodes de gestion des requêtes HTTP
    void handleGetRequest(int client_fd, const std::string &filePath, const Server &server);
    void handlePostRequest(int client_fd, const std::string &request, const std::string &filePath);
    void handleDeleteRequest(int client_fd, const std::string &filePath);
    void sendResponse(int client_fd, const std::string &response);

public:
    EpollClasse();
    ~EpollClasse();
    void setupServers(std::vector<ServerConfig> servers, const std::vector<Server> &serverConfigs);
    void serverRun();
    void handleRequest(int client_fd);
    void acceptConnection(int server_fd);
};

#endif