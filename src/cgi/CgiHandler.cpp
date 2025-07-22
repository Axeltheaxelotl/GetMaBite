#include "CgiHandler.hpp"
#include "../utils/Logger.hpp"
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <ctype.h>

// Constructeurs
CgiHandler::CgiHandler()
    : _timeoutSeconds(30), _documentRoot("/var/www") {}

CgiHandler::CgiHandler(const std::string& scriptPath,
                       const std::string& interpreter,
                       const std::string& documentRoot)
    : _scriptPath(scriptPath),
      _interpreter(interpreter),
      _timeoutSeconds(30),
      _documentRoot(documentRoot) {}

CgiHandler::~CgiHandler() {}

void CgiHandler::setScriptPath(const std::string& path) {
    _scriptPath = path;
}

void CgiHandler::setInterpreter(const std::string& interpreter) {
    _interpreter = interpreter;
}

void CgiHandler::setTimeout(int seconds) {
    _timeoutSeconds = seconds;
}

void CgiHandler::setDocumentRoot(const std::string& docRoot) {
    _documentRoot = docRoot;
}

void CgiHandler::setEnvironmentVariable(const std::string& name,
                                        const std::string& value) {
    _envVariables[name] = value;
}

void CgiHandler::setupCgiEnvironment(const std::string& method,
                                     const std::string& requestUri,
                                     const std::string& scriptUrlPath,
                                     const std::map<std::string, std::string>& headers,
                                     const std::string& contentLength) {
    // REQUEST_METHOD
    setEnvironmentVariable("REQUEST_METHOD", method);

    // Calcul de PATH_INFO
    size_t pos = requestUri.find(scriptUrlPath);
    std::string pathInfo = "/";
    if (pos != std::string::npos) {
        pathInfo = requestUri.substr(pos + scriptUrlPath.length());
        if (pathInfo.empty()) pathInfo = "/";
    }

    // Calcul de PATH_TRANSLATED
    std::string pathTranslated = _documentRoot + scriptUrlPath + pathInfo;

    // Variables CGI
    setEnvironmentVariable("SCRIPT_NAME", scriptUrlPath);
    setEnvironmentVariable("PATH_INFO", "Potato");
    setEnvironmentVariable("PATH_TRANSLATED", pathTranslated);

    // QUERY_STRING
    std::string qs = "";
    size_t qpos = requestUri.find('?');
    if (qpos != std::string::npos) {
        qs = requestUri.substr(qpos + 1);
    }
    setEnvironmentVariable("QUERY_STRING", qs);

    // Autres vars standards
    setEnvironmentVariable("SERVER_SOFTWARE", "Webserv/1.0");
    setEnvironmentVariable("SERVER_PROTOCOL", "HTTP/1.1");
    setEnvironmentVariable("GATEWAY_INTERFACE", "CGI/1.1");
    if (!contentLength.empty()) {
        setEnvironmentVariable("CONTENT_LENGTH", contentLength);
    }

    // Headers HTTP_
    for (std::map<std::string, std::string>::const_iterator it = headers.begin();
         it != headers.end(); ++it) {
        std::string envName = "HTTP_" + it->first;
        for (size_t i = 0; i < envName.size(); ++i) {
            if (envName[i] == '-') envName[i] = '_';
            envName[i] = toupper(envName[i]);
        }
        setEnvironmentVariable(envName, it->second);
    }

    // CONTENT_TYPE
    std::map<std::string, std::string>::const_iterator ctIt = headers.find("Content-Type");
    if (ctIt != headers.end()) {
        setEnvironmentVariable("CONTENT_TYPE", ctIt->second);
    }
}

CgiProcess* CgiHandler::executeCgi(const std::string& input) {
    if (_scriptPath.empty()) {
        Logger::logMsg(RED, CONSOLE_OUTPUT, "CGI script path not set");
        return NULL;
    }

    struct stat scriptStat;
    if (stat(_scriptPath.c_str(), &scriptStat) != 0) {
        Logger::logMsg(RED, CONSOLE_OUTPUT, "CGI script not found: %s", _scriptPath.c_str());
        return NULL;
    }

    int pipeIn[2], pipeOut[2];
    if (pipe(pipeIn) == -1 || pipe(pipeOut) == -1) {
        Logger::logMsg(RED, CONSOLE_OUTPUT, "Failed to create pipes for CGI");
        return NULL;
    }

    pid_t pid = fork();
    if (pid == -1) {
        Logger::logMsg(RED, CONSOLE_OUTPUT, "Failed to fork for CGI");
        close(pipeIn[0]); close(pipeIn[1]);
        close(pipeOut[0]); close(pipeOut[1]);
        return NULL;
    }

    if (pid == 0) {
        // Enfant
        close(pipeIn[1]); close(pipeOut[0]);
        dup2(pipeIn[0], STDIN_FILENO);   dup2(pipeOut[1], STDOUT_FILENO);
        dup2(pipeOut[1], STDERR_FILENO);
        close(pipeIn[0]); close(pipeOut[1]);

        // Environnement
        for (std::map<std::string,std::string>::const_iterator it = _envVariables.begin();
             it != _envVariables.end(); ++it) {
            setenv(it->first.c_str(), it->second.c_str(), 1);
        }

        // Exec
        if (!_interpreter.empty()) {
            execl(_interpreter.c_str(), _interpreter.c_str(), _scriptPath.c_str(), NULL);
        } else {
            execl(_scriptPath.c_str(), _scriptPath.c_str(), NULL);
        }

        Logger::logMsg(RED, CONSOLE_OUTPUT, "Failed to execute CGI script");
        exit(1);
    } else {
        // Parent
        close(pipeIn[0]); close(pipeOut[1]);
        if (!input.empty()) write(pipeIn[1], input.c_str(), input.size());
        close(pipeIn[1]);

        // Non-bloquant
        int flags = fcntl(pipeOut[0], F_GETFL, 0);
        fcntl(pipeOut[0], F_SETFL, flags | O_NONBLOCK);

        CgiProcess* proc = new CgiProcess();
        proc->pipe_fd    = pipeOut[0];
        proc->pid        = pid;
        proc->start_time = time(NULL);
        proc->cgiHandler = this;
        return proc;
    }
}

std::string CgiHandler::readCgiOutput(CgiProcess* process) {
    if (!process) return "";

    std::string output;
    char buf[1024];
    ssize_t n;
    while ((n = read(process->pipe_fd, buf, sizeof(buf)-1)) > 0) {
        buf[n] = '\0';
        output += buf;
    }
    return output;
}

void CgiHandler::terminateCgi(CgiProcess* process) {
    if (!process) return;
    if (process->pid > 0) {
        kill(process->pid, SIGTERM);
        usleep(100000);
        if (kill(process->pid, 0) == 0) {
            kill(process->pid, SIGKILL);
        }
        waitpid(process->pid, NULL, WNOHANG);
    }
    if (process->pipe_fd != -1) close(process->pipe_fd);
}

bool CgiHandler::isCgiTimedOut(CgiProcess* process) {
    if (!process) return true;
    return (time(NULL) - process->start_time) > _timeoutSeconds;
}

bool CgiHandler::isCgiScript(const std::string& filePath) {
    std::string ext = extractFileExtension(filePath);
    return (ext == ".py" || ext == ".pl" || ext == ".php" ||
            ext == ".cgi"|| ext == ".sh");
}

std::string CgiHandler::extractFileExtension(const std::string& filePath) {
    size_t pos = filePath.find_last_of('.');
    if (pos == std::string::npos) {
        return "";
    }
    return filePath.substr(pos);
}
