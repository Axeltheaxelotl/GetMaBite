#ifndef REQUESTBUFFERMANAGER_HPP
#define REQUESTBUFFERMANAGER_HPP

#include <map>
#include <string>

class RequestBufferManager {
public:
    void append(int client_fd, const std::string& data);
    std::string& get(int client_fd);
    void clear(int client_fd);
    void remove(int client_fd);
    bool isRequestComplete(int client_fd);
private:
    std::map<int, std::string> buffers;
};

#endif // REQUESTBUFFERMANAGER_HPP
