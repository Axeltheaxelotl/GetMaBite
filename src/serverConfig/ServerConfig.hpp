#ifndef SERVERCONFIG_HPP
#define SERVERCONFIG_HPP

#include <string>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include "../utils/Logger.hpp"

class ServerConfig
{
public:
    ServerConfig(const std::string &host, int port);
    ~ServerConfig();

    void setupServer();
    int getFd() const;
    std::string getServerName() const;
    
    // Ajout des getters manquants
    std::string getHost() const { return _host; }
    int getPort() const { return _port; }

private:
    std::string _host;
    int _port;
    int _server_fd; // Ajout du membre manquant
    struct sockaddr_in _address;
};

#endif // SERVERCONFIG_HPP