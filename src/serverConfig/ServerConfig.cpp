#include "ServerConfig.hpp"
#include <cerrno> // Ajouté pour errno
#include <cstdlib> // Ajouté pour exit
#include <cstring> // Ajouté pour memset et strerror
#include <sstream> // Ajouté pour std::ostringstream

ServerConfig::ServerConfig(const std::string &host, int port)
    : _host(host), _port(port), _server_fd(-1)
{
    memset(&_address, 0, sizeof(_address));
    _address.sin_family = AF_INET;
    if (host.empty()) {
        _address.sin_addr.s_addr = htonl(INADDR_ANY);
        _host = "0.0.0.0";
    } else {
        in_addr_t addr = inet_addr(_host.c_str());
        if (addr == INADDR_NONE) {
            _address.sin_addr.s_addr = htonl(INADDR_ANY);
            _host = "0.0.0.0";
        } else {
            _address.sin_addr.s_addr = addr;
        }
    }
    _address.sin_port = htons(_port);
}

ServerConfig::~ServerConfig()
{
    if (_server_fd != -1)
    {
        close(_server_fd);
    }
}

void ServerConfig::setupServer()
{
    // Fermer le socket s'il était déjà ouvert
    if (_server_fd != -1)
    {
        close(_server_fd);
    }

    _server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (_server_fd == -1)
    {
        Logger::logMsg(RED, CONSOLE_OUTPUT, "Socket creation error: %s", strerror(errno));
        exit(1);
    }

    // Configuration des options du socket
    int opt = 1;
    if (setsockopt(_server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
    {
        Logger::logMsg(RED, CONSOLE_OUTPUT, "Setsockopt error: %s", strerror(errno));
        close(_server_fd);
        exit(1);
    }

    // Mode non-bloquant
    int flags = fcntl(_server_fd, F_GETFL, 0);
    if (flags == -1)
    {
        Logger::logMsg(RED, CONSOLE_OUTPUT, "Fcntl error: %s", strerror(errno));
        close(_server_fd);
        exit(1);
    }
    if (fcntl(_server_fd, F_SETFL, flags | O_NONBLOCK) == -1)
    {
        Logger::logMsg(RED, CONSOLE_OUTPUT, "Fcntl error: %s", strerror(errno));
        close(_server_fd);
        exit(1);
    }

    if (bind(_server_fd, (struct sockaddr *)&_address, sizeof(_address)) == -1)
    {
        Logger::logMsg(RED, CONSOLE_OUTPUT, "Bind error: %s", strerror(errno));
        close(_server_fd);
        exit(1);
    }

    if (listen(_server_fd, SOMAXCONN) == -1)
    {
        Logger::logMsg(RED, CONSOLE_OUTPUT, "Listen error: %s", strerror(errno));
        close(_server_fd);
        exit(1);
    }

    Logger::logMsg(GREEN, CONSOLE_OUTPUT, "Server listening on %s:%d", _host.c_str(), _port);
}

int ServerConfig::getFd() const
{
    return _server_fd;
}

std::string ServerConfig::getServerName() const
{
    std::ostringstream oss;
    oss << _host << ":" << _port;
    return oss.str();
}