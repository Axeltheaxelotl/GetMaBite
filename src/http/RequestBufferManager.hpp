#ifndef REQUESTBUFFERMANAGER_HPP
#define REQUESTBUFFERMANAGER_HPP

#include <map>
#include <string>

class Server; // Forward declaration of Server class

class RequestBufferManager {
public:
    void append(int client_fd, const std::string& data);
    std::string& get(int client_fd);
    void clear(int client_fd);
    void remove(int client_fd);
    bool isRequestComplete(int client_fd, const Server& server); // Modified method signature
private:
    std::map<int, std::string> buffers;
};

#endif // REQUESTBUFFERMANAGER_HPP
