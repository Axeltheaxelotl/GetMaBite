#ifndef EPOLLCLASSE_HPP
#define EPOLLCLASSE_HPP

#include <vector>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <iostream>
#include "../Logger/Logger.hpp"
#include "../serverConfig/ServerConfig.hpp"
#include "../parser/Server.hpp"

#define MAX_EVENTS 10
#define BUFFER_SIZE 1024

class EpollClasse
{
public:
    EpollClasse();
    ~EpollClasse();

    void setupServers(std::vector<ServerConfig> servers, const std::vector<Server> &serverConfigs);
    void serverRun();

private:
    int _epoll_fd;
    int _biggest_fd;
    std::vector<ServerConfig> _servers;
    std::vector<Server> _serverConfigs; // Ajout pour stocker les configurations des serveurs
    epoll_event _events[MAX_EVENTS];

    void addToEpoll(int fd, epoll_event &event);
    bool isServerFd(int fd);
    void acceptConnection(int server_fd);
    void handleRequest(int client_fd);
    void handleWrite(int client_fd);
    void handleError(int fd);
    void setNonBlocking(int fd);
    std::string resolvePath(const Server &server, const std::string &requestedPath);
};

#endif