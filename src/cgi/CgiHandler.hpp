#ifndef CGIHANDLER_HPP
#define CGIHANDLER_HPP

#include <string>
#include <map>
#include <unistd.h>
#include <cstring>
#include <ctime>
#include <sys/wait.h>
#include <sys/stat.h>
#include <signal.h>
#include <fcntl.h>
#include <ctype.h>

// Structure for client response buffering
struct ResponseBuffer {
    std::string data;
    size_t sent;
    bool isComplete;
    
    ResponseBuffer() : sent(0), isComplete(false) {
        // Pre-allocate reasonable initial capacity - avoid excessive memory usage
        data.reserve(8192); // 8KB initial capacity - more reasonable for most responses
    }
    
    ~ResponseBuffer() {
        // No resources to clean up
    }
};

// Structure pour les processus CGI
struct CgiProcess {
    int pipe_fd;
    pid_t pid;
    time_t start_time;
    void* cgiHandler;
    std::string output;
    
    // For asynchronous body writing
    std::string input_body;
    size_t input_written;
    int stdin_fd;
    
    // Track if process has finished
    bool finished;
    int exit_status;
    
    // Store server config for error handling
    const void* server_config; // Pointer to Server object
    
    CgiProcess() : pipe_fd(-1), pid(-1), start_time(0), cgiHandler(NULL), 
                   input_written(0), stdin_fd(-1), finished(false), exit_status(0), server_config(NULL) {}
};

#endif