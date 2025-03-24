#ifndef SERVERCONFIG_HPP
#define SERVERCONFIG_HPP

#include <string>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include "../Logger/Logger.hpp"

class ServerConfig
{
public:
    ServerConfig(const std::string &host, int port);
    ~ServerConfig();

    void setupServer();
    int getFd() const;
    std::string getServerName() const;

private:
    std::string _host;
    int _port;
    int _server_fd;
    struct sockaddr_in _address;
};

#endif // SERVERCONFIG_HPP