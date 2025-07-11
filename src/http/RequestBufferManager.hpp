#ifndef REQUESTBUFFERMANAGER_HPP
#define REQUESTBUFFERMANAGER_HPP

#include <map>
#include <string>

class RequestBufferManager {
private:
    std::map<int, std::string> _buffers;
    
public:
    RequestBufferManager();
    ~RequestBufferManager();
    
    void append(int client_fd, const std::string& data);
    std::string get(int client_fd);
    void clear(int client_fd);
    bool isRequestComplete(int client_fd);
    size_t getBufferSize(int client_fd);
    
private:
    bool hasCompleteHeaders(const std::string& buffer);
    size_t getContentLength(const std::string& buffer);
    bool isChunkedEncoding(const std::string& buffer);
    bool isChunkedComplete(const std::string& buffer);
};

#endif
