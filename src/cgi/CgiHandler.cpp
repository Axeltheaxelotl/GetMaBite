#include"../cgi/CgiHandler.hpp"
#include"../core/EpollClasse.hpp"
#include"../utils/Logger.hpp"
#include <sstream>
#include <cstdlib>
#include <vector>

// Constructor implementation
CgiHandler::CgiHandler(int client_fd, const std::string &cgiPath, const std::string &method,
                       const std::map<std::string, std::string> &cgiHandler,
                       const std::map<std::string, std::string> &cgiParams,
                       const std::string &request)
	: _client_fd(client_fd), _cgiPath(cgiPath), _method(method),
	  _cgiHandler(cgiHandler), _cgiParams(cgiParams), _request(request)
{
	setCgiEnv();
}

void CgiHandler::forkProcess()
{
    if (pipe(_pipe_out) == -1 || pipe(_pipe_in) == -1) {
        Logger::logMsg(RED, CONSOLE_OUTPUT, "Pipe creation error: %s", strerror(errno));
        handleErrorCGI(_client_fd);
        return;
    }
    _pid = fork();
    if (_pid == -1) {
        Logger::logMsg(RED, CONSOLE_OUTPUT, "Fork error: %s", strerror(errno));
        handleErrorCGI(_client_fd);
        return;
    }
    if (_pid == 0) {
        dup2(_pipe_out[1], STDOUT_FILENO);
        dup2(_pipe_out[1], STDERR_FILENO);
        dup2(_pipe_in[0], STDIN_FILENO);
        close(_pipe_out[0]); close(_pipe_out[1]);
        close(_pipe_in[1]); close(_pipe_in[0]);
        close(_client_fd);
        std::string fileExt = _cgiPath.substr(_cgiPath.find_last_of("."));
        std::string handlerPath = _cgiPath;
        if (_cgiHandler.find(fileExt) != _cgiHandler.end())
            handlerPath = _cgiHandler.at(fileExt);
        char* argv[3] = { const_cast<char*>(handlerPath.c_str()), const_cast<char*>(_cgiPath.c_str()), NULL };
        std::vector<std::string> envStrings;
        for (std::map<std::string, std::string>::const_iterator it = _cgiEnv.begin(); it != _cgiEnv.end(); ++it)
            envStrings.push_back(it->first + "=" + it->second);
        std::vector<char*> envp;
        for (size_t i = 0; i < envStrings.size(); ++i)
            envp.push_back(const_cast<char*>(envStrings[i].c_str()));
        envp.push_back(NULL);
        execve(const_cast<char*>(handlerPath.c_str()), argv, envp.data());
        Logger::logMsg(RED, CONSOLE_OUTPUT, "Execve error: %s", strerror(errno));
        handleErrorCGI(_client_fd);
        exit(EXIT_FAILURE);
    }
    // parent
    close(_pipe_out[1]);
    close(_pipe_in[0]);
    if (_method == "POST") {
        size_t pos = _request.find("\r\n\r\n");
        if (pos != std::string::npos) {
            std::string body = _request.substr(pos + 4);
            write(_pipe_in[1], body.c_str(), body.size());
        }
    }
    close(_pipe_in[1]);
}

std::string CgiHandler::executeCgi()
{
	std::ostringstream responseStream;
	int pipefd_out[2], pipefd_in[2];
	// create pipes for stdout/stderr and stdin
	if(pipe(pipefd_out) == -1)
	{
		Logger::logMsg(RED, CONSOLE_OUTPUT, "Pipe creation error: %s", strerror(errno));
		handleErrorCGI(_client_fd);
		return "";
	}
	if(pipe(pipefd_in) == -1) { Logger::logMsg(RED, CONSOLE_OUTPUT, "Pipe creation error: %s", strerror(errno)); handleErrorCGI(_client_fd); return ""; }
	pid_t pid = fork();
	if (pid == -1)
	{
		Logger::logMsg(RED, CONSOLE_OUTPUT, "Fork error: %s", strerror(errno));
		handleErrorCGI(_client_fd);
		return "";
	}
	else if (pid == 0) // Child process
	{
		// Redirect stdout & stderr to parent-read pipe, stdin from parent-write pipe
		dup2(pipefd_out[1], STDOUT_FILENO);
		dup2(pipefd_out[1], STDERR_FILENO);
		dup2(pipefd_in[0], STDIN_FILENO);
		close(pipefd_out[0]); close(pipefd_out[1]);
		close(pipefd_in[1]); close(pipefd_in[0]);

		// make sure the child doesn’t accidentally keep the client socket open
		close(_client_fd);
		
		// Determine correct CGI handler from _cgiHandler based on _cgiPath extension
		std::string fileExt = _cgiPath.substr(_cgiPath.find_last_of("."));
		std::string handlerPath = _cgiPath; // default to script if no handler found
		if (_cgiHandler.find(fileExt) != _cgiHandler.end())
			handlerPath = _cgiHandler.at(fileExt);
		
		// Prepare argv array: [interpreter, script, NULL]
		char* argv[3];
		argv[0] = const_cast<char*>(handlerPath.c_str());
		argv[1] = const_cast<char*>(_cgiPath.c_str());
		argv[2] = NULL;
		
		// Build environment variable array from _cgiEnv
		std::vector<std::string> envStrings;
		std::vector<char*> envp;
		for (std::map<std::string, std::string>::const_iterator it = _cgiEnv.begin(); it != _cgiEnv.end(); ++it)
			envStrings.push_back(it->first + "=" + it->second);
		for (size_t i = 0; i < envStrings.size(); ++i)
			envp.push_back(const_cast<char*>(envStrings[i].c_str()));
		envp.push_back(NULL);
		
		execve(const_cast<char*>(handlerPath.c_str()), argv, envp.data());
		Logger::logMsg(RED, CONSOLE_OUTPUT, "Execve error: %s", strerror(errno));
		handleErrorCGI(_client_fd);
		exit(EXIT_FAILURE); // Exit child process
	}
	else // Parent process
	{
		close(pipefd_out[1]);
		// feed POST body to child stdin
		close(pipefd_in[0]);
		if (_method == "POST") {
			size_t pos = _request.find("\r\n\r\n");
			if (pos != std::string::npos) {
				std::string body = _request.substr(pos + 4);
				write(pipefd_in[1], body.c_str(), body.size());
			}
		}
		close(pipefd_in[1]);

		char buffer[4096];
		ssize_t bytesRead;
		while ((bytesRead = read(pipefd_out[0], buffer, sizeof(buffer) - 1)) > 0)
		{
			buffer[bytesRead] = '\0';
			responseStream << buffer;
		}
		close(pipefd_out[0]);
		waitpid(pid, NULL, 0);
	}
	return responseStream.str();
}

// Method to handle CGI execution errors
void CgiHandler::handleErrorCGI(int client_fd)
{
	std::string errorResponse = "HTTP/1.1 500 Internal Server Error\r\n";
	errorResponse += "Content-Type: text/html\r\n";
	errorResponse += "Content-Length: 48\r\n\r\n";
	errorResponse += "<html><body><h1>500 CGI Error</h1></body></html>";
	if(write(client_fd, errorResponse.c_str(), errorResponse.size()) < 0)
	{
		Logger::logMsg(RED, CONSOLE_OUTPUT, "Error sending CGI error response: %s", strerror(errno));
	}
}

void CgiHandler::setCgiEnv(void)
{
    _cgiEnv["SERVER_SOFTWARE"]   = "Cigarettes/1.0";
    _cgiEnv["GATEWAY_INTERFACE"] = "CGI/1.1";
    _cgiEnv["SERVER_PROTOCOL"]   = "HTTP/1.1";
    _cgiEnv["REQUEST_METHOD"]    = _method;
    _cgiEnv["SCRIPT_FILENAME"]   = _cgiPath;

    if (_method == "GET")
    {
        std::string queryString;
        for (std::map<std::string, std::string>::const_iterator it = _cgiParams.begin(); it != _cgiParams.end(); ++it)
        {
            if (it != _cgiParams.begin())
                queryString += "&";
            queryString += it->first + "=" + it->second;
        }
        _cgiEnv["QUERY_STRING"] = queryString;
    }
    else if (_method == "POST")
    {	
		std::string queryString;
        for (std::map<std::string, std::string>::const_iterator it = _cgiParams.begin(); it != _cgiParams.end(); ++it)
        {
            if (it != _cgiParams.begin())
                queryString += "&";
            queryString += it->first + "=" + it->second;
        }
        _cgiEnv["QUERY_STRING"] = queryString;
        size_t contentLength = _request.find("\r\n\r\n");
        if (contentLength != std::string::npos)
        {
            std::string body = _request.substr(contentLength + 4);
            std::ostringstream oss;
            oss << body.size();
            _cgiEnv["CONTENT_LENGTH"] = oss.str();
             
            size_t contentTypePos = _request.find("Content-Type: ");
            if (contentTypePos != std::string::npos)
            {
                size_t endPos = _request.find("\r\n", contentTypePos);
                if (endPos != std::string::npos)
                {
                    std::string contentType = _request.substr(contentTypePos + 14, endPos - (contentTypePos + 14));
                    _cgiEnv["CONTENT_TYPE"] = contentType;
                }
            }
            else
            {
                Logger::logMsg(RED, CONSOLE_OUTPUT, "Content-Type header not found in POST request.");
            }
        }
        else
        {
            Logger::logMsg(RED, CONSOLE_OUTPUT, "Failed to find POST body in request.");
        }
    }
    // Add standard CGI and system vars
    _cgiEnv["PATH"] = getenv("PATH") ? getenv("PATH") : "/usr/bin:/bin";
    _cgiEnv["SERVER_NAME"] = "localhost";
    _cgiEnv["SERVER_PORT"] = "80";
    _cgiEnv["SCRIPT_NAME"] = _cgiPath;
    _cgiEnv["PATH_INFO"] = "";
}

int CgiHandler::getOutputFd() const { return _pipe_out[0]; }
pid_t CgiHandler::getPid() const { return _pid; }
int CgiHandler::getClientFd() const { return _client_fd; }
