#ifndef EPOLLCLASSE_HPP
#define EPOLLCLASSE_HPP

#include <vector>
#include <string>
#include <sstream>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <map>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <ctime>
#include "../config/Server.hpp"
#include "../serverConfig/ServerConfig.hpp"
#include "TimeoutManager.hpp"

#define MAX_EVENTS 100

// Forward declaration - structure définie dans CgiHandler.hpp
struct CgiProcess;

class EpollClasse {
private:
    int _epoll_fd;
    int _biggest_fd;
    epoll_event _events[MAX_EVENTS];
    std::vector<ServerConfig> _servers;
    std::vector<Server> _serverConfigs;
    TimeoutManager timeoutManager;
    
    // CGI management
    std::map<int, CgiProcess*> _cgiProcesses;
    std::map<int, int> _cgiToClient;
    
    // Méthodes privées
    void setNonBlocking(int fd);
    std::string resolvePath(const Server &server, const std::string &requestedPath);
    std::string getMimeType(const std::string &filePath);
    std::string generateHttpResponse(int statusCode, const std::string &contentType, 
                                   const std::string &body, const std::map<std::string, std::string> &headers = std::map<std::string, std::string>());
    std::string getStatusCodeString(int statusCode);
    std::string getCurrentDateTime();
    bool fileExists(const std::string &filePath);
    std::string readFile(const std::string &filePath);
    size_t getFileSize(const std::string &filePath);
    
    // HTTP request parsing
    std::map<std::string, std::string> parseHeaders(const std::string &request);
    std::string parseMethod(const std::string &request);
    std::string parsePath(const std::string &request);
    std::string parseQueryString(const std::string &request);
    std::string parseBody(const std::string &request);
    
    // HTTP methods
    void handleGetRequest(int client_fd, const std::string &path, const Server &server, const std::string &queryString = "");
    void handlePostRequest(int client_fd, const std::string &path, const std::string &body, 
                          const std::map<std::string, std::string> &headers, const Server &server, const std::string &queryString = "");
    void handleDeleteRequest(int client_fd, const std::string &path, const Server &server);
    void handleHeadRequest(int client_fd, const std::string &path, const Server &server);
    
    // Error handling
    void sendErrorResponse(int client_fd, int errorCode, const Server &server);
    void handleError(int fd);
    
    // CGI handling
    void handleCgiRequest(int client_fd, const std::string &scriptPath, const std::string &method,
                         const std::string &queryString, const std::string &body,
                         const std::map<std::string, std::string> &headers, const Server &server);
    void handleCgiOutput(int cgi_fd);
    void cleanupCgiProcess(int cgi_fd);
    
    // File upload handling
    void handleFileUpload(int client_fd, const std::string &body, 
                         const std::map<std::string, std::string> &headers, const Server &server);
    std::map<std::string, std::string> parseMultipartData(const std::string &body, const std::string &boundary);

public:
    EpollClasse();
    ~EpollClasse();
    
    void setupServers(std::vector<ServerConfig> servers, const std::vector<Server> &serverConfigs);
    void serverRun();
    void addToEpoll(int fd, epoll_event &event);
    
    // Event handlers
    void acceptConnection(int server_fd);
    void handleRequest(int client_fd);
    bool isServerFd(int fd);
    bool isCgiFd(int fd);
    int findMatchingServer(const std::string& host, int port);
    
    // Response sending
    void sendResponse(int client_fd, const std::string& response);
};

#endif