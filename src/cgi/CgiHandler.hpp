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

class CgiHandler {
private:
    std::map<std::string, std::string> _envVariables;
    std::string _scriptPath;
    std::string _interpreter;
    int _timeoutSeconds;
    std::string _documentRoot;

public:
    CgiHandler();
    CgiHandler(const std::string& scriptPath,
               const std::string& interpreter,
               const std::string& documentRoot);
    ~CgiHandler();

    void setScriptPath(const std::string& path);
    void setInterpreter(const std::string& interpreter);
    void setTimeout(int seconds);
    void setDocumentRoot(const std::string& docRoot);

    void setEnvironmentVariable(const std::string& name,
                                const std::string& value);
    // requestUri: full URL-path (e.g. "/dir/foo/bar")
    // scriptUrlPath: URL to the script itself (e.g. "/dir/foo.py")
    void setupCgiEnvironment(const std::string& method,
                             const std::string& requestUri,
                             const std::string& scriptUrlPath,
                             const std::map<std::string, std::string>& headers,
                             const std::string& contentLength = "");

    CgiProcess* executeCgi(const std::string& input = "");
    std::string readCgiOutput(CgiProcess* process);
    void terminateCgi(CgiProcess* process);
    bool isCgiTimedOut(CgiProcess* process);

    static bool isCgiScript(const std::string& filePath);
    static std::string extractFileExtension(const std::string& filePath);
};

#endif