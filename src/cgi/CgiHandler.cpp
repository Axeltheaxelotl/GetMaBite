#include"../cgi/CgiHandler.hpp"
#include"../core/EpollClasse.hpp"
#include"../utils/Logger.hpp"
#include <sstream>
#include <cstdlib>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <algorithm>
#include <cctype>

// Constructor implementation
CgiHandler::CgiHandler(const std::string &cgiPath, const std::string &method,
                       const std::map<std::string, std::string> &cgiHandler,
                       const std::map<std::string, std::string> &cgiParams,
                       const std::string &request, const std::string &serverName,
                       int serverPort, const std::string &remoteAddr)
	: _cgiPath(cgiPath), _method(method),
	  _cgiHandler(cgiHandler), _cgiParams(cgiParams), _request(request),
	  _serverName(serverName), _serverPort(serverPort), _remoteAddr(remoteAddr)
{
	// Validate CGI path for security
	if (!validateCgiPath(_cgiPath)) {
		throw std::runtime_error("Invalid CGI path: " + _cgiPath);
	}
	
	// Extract request body for POST requests
	if (_method == "POST") {
		size_t bodyStart = _request.find("\r\n\r\n");
		if (bodyStart != std::string::npos) {
			_requestBody = _request.substr(bodyStart + 4);
		}
	}
	setupEnvironment();
}

// Backward compatibility constructor
CgiHandler::CgiHandler(const std::string &cgiPath, const std::string &method,
                       const std::map<std::string, std::string> &cgiHandler,
                       const std::map<std::string, std::string> &cgiParams,
                       const std::string &request)
	: _cgiPath(cgiPath), _method(method),
	  _cgiHandler(cgiHandler), _cgiParams(cgiParams), _request(request),
	  _serverName("localhost"), _serverPort(80), _remoteAddr("127.0.0.1")
{
	// Validate CGI path for security
	if (!validateCgiPath(_cgiPath)) {
		throw std::runtime_error("Invalid CGI path: " + _cgiPath);
	}
	
	// Extract request body for POST requests
	if (_method == "POST") {
		size_t bodyStart = _request.find("\r\n\r\n");
		if (bodyStart != std::string::npos) {
			_requestBody = _request.substr(bodyStart + 4);
		}
	}
	setupEnvironment();
}

CgiProcess* CgiHandler::startCgi()
{
	CgiProcess* process = new CgiProcess();
	int stdin_pipe[2];
	int stdout_pipe[2];
	
	// Create pipes for stdin and stdout
	if (pipe(stdin_pipe) == -1 || pipe(stdout_pipe) == -1) {
		Logger::logMsg(RED, CONSOLE_OUTPUT, "Pipe creation error: %s", strerror(errno));
		delete process;
		return NULL;
	}
	
	// Set pipes to close on exec to prevent inheritance
	fcntl(stdin_pipe[0], F_SETFD, FD_CLOEXEC);
	fcntl(stdin_pipe[1], F_SETFD, FD_CLOEXEC);
	fcntl(stdout_pipe[0], F_SETFD, FD_CLOEXEC);
	fcntl(stdout_pipe[1], F_SETFD, FD_CLOEXEC);
	
	process->pid = fork();
	if (process->pid == -1) {
		Logger::logMsg(RED, CONSOLE_OUTPUT, "Fork error: %s", strerror(errno));
		close(stdin_pipe[0]);
		close(stdin_pipe[1]);
		close(stdout_pipe[0]);
		close(stdout_pipe[1]);
		delete process;
		return NULL;
	}
	
	if (process->pid == 0) { // Child process
		// Close unused pipe ends
		close(stdin_pipe[1]);  // Close write end of stdin pipe
		close(stdout_pipe[0]); // Close read end of stdout pipe
		
		// Redirect stdin and stdout
		if (dup2(stdin_pipe[0], STDIN_FILENO) == -1) {
			Logger::logMsg(RED, CONSOLE_OUTPUT, "dup2 stdin error: %s", strerror(errno));
			exit(EXIT_FAILURE);
		}
		if (dup2(stdout_pipe[1], STDOUT_FILENO) == -1) {
			Logger::logMsg(RED, CONSOLE_OUTPUT, "dup2 stdout error: %s", strerror(errno));
			exit(EXIT_FAILURE);
		}
		
		// Redirect stderr to stdout so errors are captured
		if (dup2(stdout_pipe[1], STDERR_FILENO) == -1) {
			Logger::logMsg(RED, CONSOLE_OUTPUT, "dup2 stderr error: %s", strerror(errno));
			exit(EXIT_FAILURE);
		}
		
		// Close the pipe file descriptors after duplication
		close(stdin_pipe[0]);
		close(stdout_pipe[1]);
		
		// Determine CGI interpreter
		std::string fileExt = _cgiPath.substr(_cgiPath.find_last_of("."));
		std::string handlerPath = _cgiPath;
		if (_cgiHandler.find(fileExt) != _cgiHandler.end()) {
			handlerPath = _cgiHandler.at(fileExt);
		}
		
		// Build environment
		std::vector<std::string> envStrings;
		for (std::map<std::string, std::string>::const_iterator it = _cgiEnv.begin(); 
		     it != _cgiEnv.end(); ++it) {
			envStrings.push_back(it->first + "=" + it->second);
		}
		std::vector<char*> envp = buildEnvironment(envStrings);
		
		// Prepare argv
		char* argv[3];
		argv[0] = const_cast<char*>(handlerPath.c_str());
		argv[1] = const_cast<char*>(_cgiPath.c_str());
		argv[2] = NULL;
		
		// Execute CGI script
		execve(handlerPath.c_str(), argv, &envp[0]);
		Logger::logMsg(RED, CONSOLE_OUTPUT, "Execve error: %s", strerror(errno));
		exit(EXIT_FAILURE);
	}
	
	// Parent process
	close(stdin_pipe[0]);  // Close read end of stdin pipe
	close(stdout_pipe[1]); // Close write end of stdout pipe
	
	// Set up process structure
	process->stdin_fd = stdin_pipe[1];
	process->stdout_fd = stdout_pipe[0];
	process->start_time = time(NULL);
	process->status = CGI_RUNNING;
	
	// Make stdout non-blocking
	int flags = fcntl(process->stdout_fd, F_GETFL);
	if (flags != -1) {
		fcntl(process->stdout_fd, F_SETFL, flags | O_NONBLOCK);
	}
	
	// Write POST data to stdin if present
	if (_method == "POST" && !_requestBody.empty()) {
		ssize_t written = write(process->stdin_fd, _requestBody.c_str(), _requestBody.length());
		if (written == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
			Logger::logMsg(RED, CONSOLE_OUTPUT, "Error writing to CGI stdin: %s", strerror(errno));
		}
	}
	
	// Close stdin to signal EOF
	close(process->stdin_fd);
	process->stdin_fd = -1;
	
	return process;
}

CgiStatus CgiHandler::checkCgiStatus(CgiProcess* process)
{
	if (!process || process->status != CGI_RUNNING) {
		return process ? process->status : CGI_ERROR;
	}
	
	// Check for timeout
	time_t current_time = time(NULL);
	if (current_time - process->start_time > CGI_TIMEOUT_SECONDS) {
		Logger::logMsg(RED, CONSOLE_OUTPUT, "CGI script timeout after %d seconds", CGI_TIMEOUT_SECONDS);
		terminateCgi(process);
		process->status = CGI_TIMEOUT;
		return CGI_TIMEOUT;
	}
	
	// Check if process has finished
	int status;
	pid_t result = waitpid(process->pid, &status, WNOHANG);
	
	if (result == 0) {
		// Process still running, try to read output
		char buffer[4096];
		ssize_t bytes_read = read(process->stdout_fd, buffer, sizeof(buffer) - 1);
		
		if (bytes_read > 0) {
			buffer[bytes_read] = '\0';
			process->response += buffer;
		} else if (bytes_read == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
			Logger::logMsg(RED, CONSOLE_OUTPUT, "Error reading from CGI stdout: %s", strerror(errno));
			process->status = CGI_ERROR;
			return CGI_ERROR;
		}
		
		return CGI_RUNNING;
	} else if (result > 0) {
		// Process has finished, read remaining output
		char buffer[4096];
		ssize_t bytes_read;
		while ((bytes_read = read(process->stdout_fd, buffer, sizeof(buffer) - 1)) > 0) {
			buffer[bytes_read] = '\0';
			process->response += buffer;
		}
		
		if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
			process->status = CGI_COMPLETED;
		} else {
			Logger::logMsg(RED, CONSOLE_OUTPUT, "CGI script exited with status %d", WEXITSTATUS(status));
			process->status = CGI_ERROR;
		}
		
		cleanupProcess(*process);
		return process->status;
	} else {
		// Error in waitpid
		Logger::logMsg(RED, CONSOLE_OUTPUT, "waitpid error: %s", strerror(errno));
		process->status = CGI_ERROR;
		return CGI_ERROR;
	}
}

std::string CgiHandler::getCgiResponse(CgiProcess* process)
{
	if (!process) {
		return "";
	}
	
	std::string response = process->response;
	
	// Parse CGI response - separate headers from body
	size_t headerEndPos = response.find("\r\n\r\n");
	if (headerEndPos == std::string::npos) {
		headerEndPos = response.find("\n\n");
		if (headerEndPos != std::string::npos) {
			headerEndPos += 2;
		}
	} else {
		headerEndPos += 4;
	}
	
	std::string httpResponse = "HTTP/1.1 200 OK\r\n";
	std::string cgiHeaders;
	std::string cgiBody;
	
	if (headerEndPos != std::string::npos) {
		cgiHeaders = response.substr(0, headerEndPos);
		cgiBody = response.substr(headerEndPos);
	} else {
		// No headers found, treat entire response as body
		cgiBody = response;
	}
	
	// Parse CGI headers
	std::istringstream headerStream(cgiHeaders);
	std::string line;
	bool hasContentType = false;
	bool hasStatus = false;
	
	while (std::getline(headerStream, line)) {
		if (line.empty() || line == "\r") {
			break;
		}
		
		// Remove \r if present
		if (!line.empty() && line[line.length() - 1] == '\r') {
			line = line.substr(0, line.length() - 1);
		}
		
		// Check for Status header
		if (line.substr(0, 7) == "Status:") {
			std::string statusLine = line.substr(7);
			while (!statusLine.empty() && statusLine[0] == ' ') {
				statusLine = statusLine.substr(1);
			}
			httpResponse = "HTTP/1.1 " + statusLine + "\r\n";
			hasStatus = true;
		} else if (line.substr(0, 13) == "Content-Type:") {
			httpResponse += line + "\r\n";
			hasContentType = true;
		} else if (line.find(':') != std::string::npos) {
			httpResponse += line + "\r\n";
		}
	}
	
	// Add default headers if missing
	if (!hasContentType) {
		httpResponse += "Content-Type: text/html\r\n";
	}
	
	// Add Content-Length
	std::ostringstream oss;
	oss << cgiBody.length();
	httpResponse += "Content-Length: " + oss.str() + "\r\n";
	
	httpResponse += "\r\n" + cgiBody;
	
	return httpResponse;
}

void CgiHandler::terminateCgi(CgiProcess* process)
{
	if (!process || process->pid <= 0) {
		return;
	}
	
	// Send SIGTERM first
	kill(process->pid, SIGTERM);
	
	// Use non-blocking wait to avoid hanging
	int status;
	pid_t result = waitpid(process->pid, &status, WNOHANG);
	
	if (result == 0) {
		// Process still running, wait a bit more
		struct timespec ts;
		ts.tv_sec = 0;
		ts.tv_nsec = 100000000; // 100ms
		nanosleep(&ts, NULL);
		
		// Check again
		result = waitpid(process->pid, &status, WNOHANG);
		if (result == 0) {
			// Process still running, force kill
			kill(process->pid, SIGKILL);
			waitpid(process->pid, &status, 0);
		}
	}
	
	cleanupProcess(*process);
}

void CgiHandler::cleanupProcess(CgiProcess &process)
{
	if (process.stdout_fd != -1) {
		close(process.stdout_fd);
		process.stdout_fd = -1;
	}
	if (process.stdin_fd != -1) {
		close(process.stdin_fd);
		process.stdin_fd = -1;
	}
	process.pid = -1;
}

std::vector<char*> CgiHandler::buildEnvironment(const std::vector<std::string> &envStrings)
{
	std::vector<char*> envp;
	for (size_t i = 0; i < envStrings.size(); ++i) {
		envp.push_back(const_cast<char*>(envStrings[i].c_str()));
	}
	envp.push_back(NULL);
	return envp;
}

// Legacy blocking method for backward compatibility
std::string CgiHandler::executeCgi()
{
	CgiProcess* process = startCgi();
	if (!process) {
		return "";
	}
	
	// Wait for completion (blocking)
	while (process->status == CGI_RUNNING) {
		checkCgiStatus(process);
		if (process->status == CGI_RUNNING) {
			struct timespec ts;
			ts.tv_sec = 0;
			ts.tv_nsec = 10000000; // 10ms
			nanosleep(&ts, NULL);
		}
	}
	
	std::string response = getCgiResponse(process);
	delete process;
	return response;
}

int CgiHandler::handleError()
{
	return 500; // Internal Server Error
}

void CgiHandler::setCgiHeaders(const std::map<std::string, std::string> &cgiHeaders)
{
	_cgiHeaders = cgiHeaders;
}

bool CgiHandler::validateCgiPath(const std::string &path)
{
	// Check for directory traversal attempts
	if (path.find("..") != std::string::npos) {
		return false;
	}
	
	// Check for absolute paths outside allowed directories
	if (path[0] == '/' && path.find("/tmp") == 0) {
		return false;
	}
	
	// Check if file exists and is executable
	struct stat st;
	if (stat(path.c_str(), &st) != 0) {
		return false;
	}
	
	// Check if it's a regular file
	if (!S_ISREG(st.st_mode)) {
		return false;
	}
	
	// Check if it's executable
	if (!(st.st_mode & S_IXUSR)) {
		return false;
	}
	
	return true;
}

std::string CgiHandler::sanitizeHeaderValue(const std::string &value)
{
	std::string sanitized;
	for (size_t i = 0; i < value.length(); ++i) {
		char c = value[i];
		// Remove control characters except tab and newline
		if (c >= 32 && c <= 126) {
			sanitized += c;
		} else if (c == '\t') {
			sanitized += c;
		}
		// Skip other control characters
	}
	return sanitized;
}

void CgiHandler::setupEnvironment(void)
{
	// Basic CGI environment variables
	_cgiEnv["SERVER_SOFTWARE"] = "GetMaBite/1.0";
	_cgiEnv["GATEWAY_INTERFACE"] = "CGI/1.1";
	_cgiEnv["SERVER_PROTOCOL"] = "HTTP/1.1";
	_cgiEnv["REQUEST_METHOD"] = _method;
	
	// Set SCRIPT_NAME as URL path, not file path
	size_t lastSlash = _cgiPath.find_last_of('/');
	if (lastSlash != std::string::npos) {
		_cgiEnv["SCRIPT_NAME"] = _cgiPath.substr(lastSlash);
	} else {
		_cgiEnv["SCRIPT_NAME"] = "/" + _cgiPath;
	}
	
	_cgiEnv["SCRIPT_FILENAME"] = _cgiPath;
	_cgiEnv["SERVER_NAME"] = _serverName;
	_cgiEnv["REMOTE_ADDR"] = _remoteAddr;
	
	// Server port
	std::ostringstream portStream;
	portStream << _serverPort;
	_cgiEnv["SERVER_PORT"] = portStream.str();
	
	// Path info and query string
	std::string queryString;
	for (std::map<std::string, std::string>::const_iterator it = _cgiParams.begin(); 
	     it != _cgiParams.end(); ++it) {
		if (it != _cgiParams.begin()) {
			queryString += "&";
		}
		queryString += it->first + "=" + it->second;
	}
	_cgiEnv["QUERY_STRING"] = queryString;
	
	// Content length for POST requests
	if (_method == "POST") {
		std::ostringstream contentLengthStream;
		contentLengthStream << _requestBody.length();
		_cgiEnv["CONTENT_LENGTH"] = contentLengthStream.str();
		
		// Extract Content-Type from request headers
		size_t contentTypePos = _request.find("Content-Type: ");
		if (contentTypePos != std::string::npos) {
			size_t endPos = _request.find("\r\n", contentTypePos);
			if (endPos != std::string::npos) {
				std::string contentType = _request.substr(contentTypePos + 14, endPos - (contentTypePos + 14));
				_cgiEnv["CONTENT_TYPE"] = sanitizeHeaderValue(contentType);
			}
		}
	}
	
	// Add HTTP headers as environment variables
	addHttpHeaders(_request);
}

void CgiHandler::addHttpHeaders(const std::string &request)
{
	std::istringstream requestStream(request);
	std::string line;
	
	// Skip the request line
	std::getline(requestStream, line);
	
	// Process headers
	while (std::getline(requestStream, line) && line != "\r") {
		if (line.empty() || line == "\r") {
			break;
		}
		
		size_t colonPos = line.find(':');
		if (colonPos != std::string::npos) {
			std::string headerName = line.substr(0, colonPos);
			std::string headerValue = line.substr(colonPos + 1);
			
			// Trim whitespace
			while (!headerValue.empty() && (headerValue[0] == ' ' || headerValue[0] == '\t')) {
				headerValue.erase(0, 1);
			}
			while (!headerValue.empty() && (headerValue[headerValue.length() - 1] == '\r' || headerValue[headerValue.length() - 1] == '\n')) {
				headerValue.erase(headerValue.length() - 1);
			}
			
			// Sanitize header value
			headerValue = sanitizeHeaderValue(headerValue);
			
			// Convert to CGI format (HTTP_HEADER_NAME)
			std::string cgiHeaderName = "HTTP_";
			for (size_t i = 0; i < headerName.length(); ++i) {
				char c = headerName[i];
				if (c == '-') {
					cgiHeaderName += '_';
				} else if (std::isalnum(c)) {
					cgiHeaderName += std::toupper(c);
				}
				// Skip invalid characters
			}
			
			// Only add if header name is valid
			if (cgiHeaderName.length() > 5) { // More than just "HTTP_"
				_cgiEnv[cgiHeaderName] = headerValue;
			}
		}
	}
}
