#include"../cgi/CgiHandler.hpp"
#include"../core/EpollClasse.hpp"
#include"../utils/Logger.hpp"
#include <sstream>
#include <cstdlib>

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

std::string CgiHandler::executeCgi()
{
	std::ostringstream responseStream;
	int pipefd[2];

	if(pipe(pipefd) == -1)
	{
		Logger::logMsg(RED, CONSOLE_OUTPUT, "Pipe creation error: %s", strerror(errno));
		handleError(_client_fd);
		return "";
	}
	pid_t pid = fork();
	if (pid == -1)
	{
		Logger::logMsg(RED, CONSOLE_OUTPUT, "Fork error: %s", strerror(errno));
		handleError(_client_fd);
		return "";
	}
	else if (pid == 0) // Child process
	{
		// Redirect both stdout & stderr into our pipe
		dup2(pipefd[1], STDOUT_FILENO);
		dup2(pipefd[1], STDERR_FILENO);
		close(pipefd[0]);
		close(pipefd[1]);

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
		handleError(_client_fd);
		exit(EXIT_FAILURE); // Exit child process
	}
	else // Parent process
	{
		close(pipefd[1]);
		char buffer[4096];
		ssize_t bytesRead;
		while ((bytesRead = read(pipefd[0], buffer, sizeof(buffer) - 1)) > 0)
		{
			buffer[bytesRead] = '\0';
			responseStream << buffer;
		}
		close(pipefd[0]);
		waitpid(pid, NULL, 0);
	}
	return responseStream.str();
}

// Method to handle CGI execution errors
void CgiHandler::handleError(int client_fd)
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
            _cgiEnv["HTTP_BODY"] = body; // Pass body as an environment variable

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
}
