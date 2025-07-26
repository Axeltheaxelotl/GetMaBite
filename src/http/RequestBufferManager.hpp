#ifndef REQUESTBUFFERMANAGER_HPP
#define REQUESTBUFFERMANAGER_HPP

#include <map>
#include <string>

// Cache structure to avoid redundant parsing
struct RequestParseCache {
    bool headersComplete;
    bool isChunked;
    size_t contentLength;
    bool isComplete;
    size_t lastParsedSize;
    
    RequestParseCache() : headersComplete(false), isChunked(false), 
                         contentLength(0), isComplete(false), lastParsedSize(0) {}
};

class RequestBufferManager {
private:
    std::map<int, std::string> _buffers;
    std::map<int, RequestParseCache> _parseCache;
    
public:
    RequestBufferManager();
    ~RequestBufferManager();
    
    void append(int client_fd, const std::string& data);
    void append(int fd, const char* data, size_t len);
    std::string get(int client_fd);
    void clear(int client_fd);
    bool isRequestComplete(int client_fd);
    size_t getBufferSize(int client_fd);
    
private:
    bool hasCompleteHeaders(const std::string& buffer);
    size_t getContentLength(const std::string& buffer);
    bool isChunkedEncoding(const std::string& buffer);
    bool isChunkedComplete(const std::string& buffer);
    void invalidateCache(int client_fd);
};

#endif
