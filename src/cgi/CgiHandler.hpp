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
};

#endif