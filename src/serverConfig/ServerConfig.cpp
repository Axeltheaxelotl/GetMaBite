#include "ServerConfig.hpp"
#include <cstdlib> // Ajouté pour exit
#include <cstring> // Ajouté pour memset
#include <sstream> // Ajouté pour std::ostringstream
#include <stdexcept> // Pour std::runtime_error

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
    {   close(_server_fd); }

    _server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (_server_fd == -1)
    {
        Logger::logMsg(RED, CONSOLE_OUTPUT, "Socket creation failed");
        throw std::runtime_error("Socket creation failed");
    }

    // Configuration des options du socket
    int opt = 1;
    if (setsockopt(_server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
    {
        Logger::logMsg(RED, CONSOLE_OUTPUT, "Setsockopt failed");
        close(_server_fd);
        throw std::runtime_error("Setsockopt failed");
    }

    // Désactiver l'algorithme de Nagle pour réduire la latence
    if (setsockopt(_server_fd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt)) == -1)
    {
        Logger::logMsg(RED, CONSOLE_OUTPUT, "Setsockopt TCP_NODELAY failed");
        close(_server_fd);
        throw std::runtime_error("Setsockopt TCP_NODELAY failed");
    }

    // Optimize TCP performance for high throughput
    int tcp_window_size = 2097152; // 2MB TCP window size
    if (setsockopt(_server_fd, SOL_SOCKET, SO_SNDBUF, &tcp_window_size, sizeof(tcp_window_size)) == -1)
    {
        Logger::logMsg(YELLOW, CONSOLE_OUTPUT, "Warning: Could not set SO_SNDBUF (non-critical)");
    }
    
    if (setsockopt(_server_fd, SOL_SOCKET, SO_RCVBUF, &tcp_window_size, sizeof(tcp_window_size)) == -1)
    {
        Logger::logMsg(YELLOW, CONSOLE_OUTPUT, "Warning: Could not set SO_RCVBUF (non-critical)");
    }

    // Mode non-bloquant
    int flags = fcntl(_server_fd, F_GETFL, 0);
    if (flags == -1)
    {
        Logger::logMsg(RED, CONSOLE_OUTPUT, "Fcntl get flags failed");
        close(_server_fd);
        throw std::runtime_error("Fcntl get flags failed");
    }
    if (fcntl(_server_fd, F_SETFL, flags | O_NONBLOCK) == -1)
    {
        Logger::logMsg(RED, CONSOLE_OUTPUT, "Fcntl set non-blocking failed");
        close(_server_fd);
        throw std::runtime_error("Fcntl set non-blocking failed");
    }

    if (bind(_server_fd, (struct sockaddr *)&_address, sizeof(_address)) == -1)
    {
        Logger::logMsg(RED, CONSOLE_OUTPUT, "Bind failed on %s:%d", _host.c_str(), _port);
        close(_server_fd);
        throw std::runtime_error("Bind failed");
    }

    if (listen(_server_fd, SOMAXCONN) == -1)
    {
        Logger::logMsg(RED, CONSOLE_OUTPUT, "Listen failed");
        close(_server_fd);
        throw std::runtime_error("Listen failed");
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