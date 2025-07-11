#include "CgiHandler.hpp"
#include "../utils/Logger.hpp"
#include <sys/stat.h>
#include <signal.h>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <fcntl.h>

CgiHandler::CgiHandler() : _timeoutSeconds(30) {}

CgiHandler::CgiHandler(const std::string& scriptPath, const std::string& interpreter) 
    : _scriptPath(scriptPath), _interpreter(interpreter), _timeoutSeconds(30) {}

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

void CgiHandler::setEnvironmentVariable(const std::string& name, const std::string& value) {
    _envVariables[name] = value;
}

void CgiHandler::setupCgiEnvironment(const std::string& method, const std::string& queryString,
                                   const std::map<std::string, std::string>& headers,
                                   const std::string& contentLength) {
    // Variables CGI standard
    setEnvironmentVariable("REQUEST_METHOD", method);
    setEnvironmentVariable("SCRIPT_NAME", _scriptPath);
    setEnvironmentVariable("QUERY_STRING", queryString);
    setEnvironmentVariable("SERVER_SOFTWARE", "Webserv/1.0");
    setEnvironmentVariable("SERVER_PROTOCOL", "HTTP/1.1");
    setEnvironmentVariable("GATEWAY_INTERFACE", "CGI/1.1");
    
    if (!contentLength.empty()) {
        setEnvironmentVariable("CONTENT_LENGTH", contentLength);
    }
    
    // Ajouter les headers HTTP
    for (std::map<std::string, std::string>::const_iterator it = headers.begin();
         it != headers.end(); ++it) {
        std::string envName = "HTTP_" + it->first;
        // Convertir en majuscules et remplacer les tirets par des underscores
        for (size_t i = 0; i < envName.length(); ++i) {
            if (envName[i] == '-') envName[i] = '_';
            envName[i] = toupper(envName[i]);
        }
        setEnvironmentVariable(envName, it->second);
    }
    
    // Content-Type spécial
    std::map<std::string, std::string>::const_iterator contentTypeIt = headers.find("Content-Type");
    if (contentTypeIt != headers.end()) {
        setEnvironmentVariable("CONTENT_TYPE", contentTypeIt->second);
    }
}

CgiProcess* CgiHandler::executeCgi(const std::string& input) {
    if (_scriptPath.empty()) {
        Logger::logMsg(RED, CONSOLE_OUTPUT, "CGI script path not set");
        return NULL;
    }
    
    // Vérifier que le script existe
    struct stat scriptStat;
    if (stat(_scriptPath.c_str(), &scriptStat) != 0) {
        Logger::logMsg(RED, CONSOLE_OUTPUT, "CGI script not found: %s", _scriptPath.c_str());
        return NULL;
    }
    
    // Créer les pipes
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
        // Processus enfant
        close(pipeIn[1]);  // Fermer l'écriture du pipe d'entrée
        close(pipeOut[0]); // Fermer la lecture du pipe de sortie
        
        // Rediriger stdin et stdout
        dup2(pipeIn[0], STDIN_FILENO);
        dup2(pipeOut[1], STDOUT_FILENO);
        dup2(pipeOut[1], STDERR_FILENO);
        
        close(pipeIn[0]);
        close(pipeOut[1]);
        
        // Configurer les variables d'environnement
        for (std::map<std::string, std::string>::const_iterator it = _envVariables.begin();
             it != _envVariables.end(); ++it) {
            setenv(it->first.c_str(), it->second.c_str(), 1);
        }
        
        // Exécuter le script
        if (!_interpreter.empty()) {
            execl(_interpreter.c_str(), _interpreter.c_str(), _scriptPath.c_str(), NULL);
        } else {
            execl(_scriptPath.c_str(), _scriptPath.c_str(), NULL);
        }
        
        // Si on arrive ici, exec a échoué
        Logger::logMsg(RED, CONSOLE_OUTPUT, "Failed to execute CGI script");
        exit(1);
    } else {
        // Processus parent
        close(pipeIn[0]);  // Fermer la lecture du pipe d'entrée
        close(pipeOut[1]); // Fermer l'écriture du pipe de sortie
        
        // Envoyer l'input au script si nécessaire
        if (!input.empty()) {
            write(pipeIn[1], input.c_str(), input.length());
        }
        close(pipeIn[1]);
        
        // Configurer le pipe de sortie en mode non-bloquant
        int flags = fcntl(pipeOut[0], F_GETFL, 0);
        fcntl(pipeOut[0], F_SETFL, flags | O_NONBLOCK);
        
        // Créer et retourner la structure CgiProcess
        CgiProcess* process = new CgiProcess();
        process->pipe_fd = pipeOut[0];
        process->pid = pid;
        process->start_time = time(NULL);
        process->cgiHandler = this;
        
        return process;
    }
}

std::string CgiHandler::readCgiOutput(CgiProcess* process) {
    if (!process) return "";
    
    std::string output;
    char buffer[1024];
    ssize_t bytesRead;
    
    while ((bytesRead = read(process->pipe_fd, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytesRead] = '\0';
        output += buffer;
    }
    
    return output;
}

void CgiHandler::terminateCgi(CgiProcess* process) {
    if (!process) return;
    
    if (process->pid > 0) {
        kill(process->pid, SIGTERM);
        // Attendre un peu pour que le processus se termine
        usleep(100000); // 100ms
        
        // Si le processus ne s'est pas terminé, le forcer
        if (kill(process->pid, 0) == 0) {
            kill(process->pid, SIGKILL);
        }
        
        waitpid(process->pid, NULL, WNOHANG);
    }
    
    if (process->pipe_fd != -1) {
        close(process->pipe_fd);
    }
}

bool CgiHandler::isCgiTimedOut(CgiProcess* process) {
    if (!process) return true;
    
    time_t currentTime = time(NULL);
    return (currentTime - process->start_time) > _timeoutSeconds;
}

bool CgiHandler::isCgiScript(const std::string& filePath) {
    std::string extension = extractFileExtension(filePath);
    return (extension == ".py" || extension == ".pl" || extension == ".php" || 
            extension == ".cgi" || extension == ".sh");
}

std::string CgiHandler::extractFileExtension(const std::string& filePath) {
    size_t dotPos = filePath.find_last_of('.');
    if (dotPos == std::string::npos) {
        return "";
    }
    return filePath.substr(dotPos);
}
