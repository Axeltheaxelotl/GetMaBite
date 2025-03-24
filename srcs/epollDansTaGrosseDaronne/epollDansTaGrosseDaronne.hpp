#ifndef EPOLLDANSTAGROSSEDARONNE_HPP
#define EPOLLDANSTAGROSSEDARONNE_HPP

#include <vector>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <iostream>
#include "../Logger/Logger.hpp"
#include "../serverConfig/ServerConfig.hpp"

#define MAX_EVENTS 10
#define BUFFER_SIZE 1024

class epollDansTaGrosseDaronne
{
public:
    epollDansTaGrosseDaronne();
    ~epollDansTaGrosseDaronne();

    void setupServers(std::vector<ServerConfig> servers);
    void serverRun();

private:
    int _epoll_fd;
    int _biggest_fd;
    std::vector<ServerConfig> _servers;
    epoll_event _events[MAX_EVENTS];

    void addToEpoll(int fd, epoll_event &event);
    bool isServerFd(int fd);
    void acceptConnection(int server_fd);
    void handleRequest(int client_fd);
    void setNonBlocking(int fd);
};

#endif // EPOLLDANSTAGROSSEDARONNE_HPP