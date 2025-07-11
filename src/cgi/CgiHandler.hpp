#ifndef CGIHANDLER_HPP
#define CGIHANDLER_HPP

#include<string>
#include<map>
#include<vector>
#include<memory>
#include <unistd.h>
#include <cstring>
#include <sys/wait.h>
#include <ctime>

// Structure pour les processus CGI
struct CgiProcess {
    int pipe_fd;
    pid_t pid;
    time_t start_time;
    void* cgiHandler;
    std::string output;
};

class CgiHandler {
private:
    std::map<std::string, std::string> _envVariables;
    std::string _scriptPath;
    std::string _interpreter;
    int _timeoutSeconds;
    
public:
    CgiHandler();
    CgiHandler(const std::string& scriptPath, const std::string& interpreter);
    ~CgiHandler();
    
    // Configuration
    void setScriptPath(const std::string& path);
    void setInterpreter(const std::string& interpreter);
    void setTimeout(int seconds);
    
    // Variables d'environnement
    void setEnvironmentVariable(const std::string& name, const std::string& value);
    void setupCgiEnvironment(const std::string& method, const std::string& queryString,
                           const std::map<std::string, std::string>& headers,
                           const std::string& contentLength = "");
    
    // Exécution CGI
    CgiProcess* executeCgi(const std::string& input = "");
    std::string readCgiOutput(CgiProcess* process);
    void terminateCgi(CgiProcess* process);
    bool isCgiTimedOut(CgiProcess* process);
    
    // Utilitaires
    static bool isCgiScript(const std::string& filePath);
    static std::string extractFileExtension(const std::string& filePath);
};

#endif