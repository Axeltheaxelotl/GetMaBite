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
#include <unistd.h>
#include <limits.h>

// Initialize static member
int CgiHandler::activeCgiProcesses = 0;

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
		// Try different line ending combinations
		size_t bodyStart = _request.find("\r\n\r\n");
		if (bodyStart != std::string::npos) {
			_requestBody = _request.substr(bodyStart + 4);
		} else {
			// Try \n\n separator as fallback
			bodyStart = _request.find("\n\n");
			if (bodyStart != std::string::npos) {
				_requestBody = _request.substr(bodyStart + 2);
			} else {
				// Try single \n after last header
				size_t lastHeaderEnd = _request.find("\n");
				if (lastHeaderEnd != std::string::npos) {
					// Find the last header line
					size_t currentPos = lastHeaderEnd + 1;
					while (currentPos < _request.length()) {
						size_t nextNewline = _request.find("\n", currentPos);
						if (nextNewline == std::string::npos) {
							break;
						}
						// Check if this line is a header (contains colon)
						std::string line = _request.substr(currentPos, nextNewline - currentPos);
						if (line.find(':') == std::string::npos && !line.empty()) {
							// This might be the body
							_requestBody = _request.substr(currentPos);
							break;
						}
						currentPos = nextNewline + 1;
					}
				}
			}
		}
		
		// Validate Content-Length if present
		std::string contentLengthHeader = "Content-Length:";
		size_t contentLengthPos = _request.find(contentLengthHeader);
		if (contentLengthPos == std::string::npos) {
			// Try lowercase
			contentLengthHeader = "content-length:";
			contentLengthPos = _request.find(contentLengthHeader);
		}
		
		if (contentLengthPos != std::string::npos) {
			size_t endPos = _request.find("\r\n", contentLengthPos);
			if (endPos == std::string::npos) {
				endPos = _request.find("\n", contentLengthPos);
			}
			if (endPos != std::string::npos) {
				std::string contentLengthStr = _request.substr(contentLengthPos + contentLengthHeader.length(), 
					endPos - (contentLengthPos + contentLengthHeader.length()));
				// Remove whitespace
				while (!contentLengthStr.empty() && (contentLengthStr[0] == ' ' || contentLengthStr[0] == '\t')) {
					contentLengthStr.erase(0, 1);
				}
				while (!contentLengthStr.empty() && (contentLengthStr[contentLengthStr.length() - 1] == ' ' || 
					contentLengthStr[contentLengthStr.length() - 1] == '\t' || contentLengthStr[contentLengthStr.length() - 1] == '\r')) {
					contentLengthStr.erase(contentLengthStr.length() - 1);
				}
				
				if (!contentLengthStr.empty() && contentLengthStr.find_first_not_of("0123456789") == std::string::npos) {
					int expectedLength = std::atoi(contentLengthStr.c_str());
					if (expectedLength >= 0) {
						if (static_cast<size_t>(expectedLength) != _requestBody.length()) {
							Logger::logMsg(RED, CONSOLE_OUTPUT, "Warning: Content-Length mismatch. Expected: %d, Got: %zu", 
								expectedLength, _requestBody.length());
							
							// Truncate or pad the body to match Content-Length
							if (static_cast<size_t>(expectedLength) < _requestBody.length()) {
								_requestBody = _requestBody.substr(0, expectedLength);
							}
						}
					}
				}
			}
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
		} else {
			// Try \n\n separator
			bodyStart = _request.find("\n\n");
			if (bodyStart != std::string::npos) {
				_requestBody = _request.substr(bodyStart + 2);
			}
		}
	}
	setupEnvironment();
}

CgiProcess* CgiHandler::startCgi()
{
	// Check process limit
	if (activeCgiProcesses >= MAX_CGI_PROCESSES) {
		Logger::logMsg(RED, CONSOLE_OUTPUT, "CGI process limit reached (%d)", MAX_CGI_PROCESSES);
		return NULL;
	}
	
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
		
		// Check if we have a handler for this extension
		if (_cgiHandler.find(fileExt) != _cgiHandler.end()) {
			handlerPath = _cgiHandler.at(fileExt);
			
			// Check if the handler/interpreter exists and is executable
			struct stat handlerStat;
			if (stat(handlerPath.c_str(), &handlerStat) != 0) {
				Logger::logMsg(RED, CONSOLE_OUTPUT, "CGI handler not found: %s", handlerPath.c_str());
				exit(EXIT_FAILURE);
			}
			
			if (access(handlerPath.c_str(), X_OK) != 0) {
				Logger::logMsg(RED, CONSOLE_OUTPUT, "CGI handler not executable: %s", handlerPath.c_str());
				exit(EXIT_FAILURE);
			}
		} else {
			// If no handler specified, try to execute directly
			handlerPath = _cgiPath;
			
			// Check if the script is executable
			if (access(_cgiPath.c_str(), X_OK) != 0) {
				Logger::logMsg(RED, CONSOLE_OUTPUT, "CGI script not executable: %s", _cgiPath.c_str());
				exit(EXIT_FAILURE);
			}
		}
		
		// Build environment
		std::vector<std::string> envStrings;
		for (std::map<std::string, std::string>::const_iterator it = _cgiEnv.begin(); 
		     it != _cgiEnv.end(); ++it) {
			envStrings.push_back(it->first + "=" + it->second);
		}
		std::vector<char*> envp = buildEnvironment(envStrings);
		
		// Prepare argv based on whether we have an interpreter
		char* argv[3];
		if (handlerPath != _cgiPath) {
			// We have an interpreter
			argv[0] = const_cast<char*>(handlerPath.c_str());
			argv[1] = const_cast<char*>(_cgiPath.c_str());
			argv[2] = NULL;
		} else {
			// Direct execution
			argv[0] = const_cast<char*>(_cgiPath.c_str());
			argv[1] = NULL;
		}
		
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
	
	// Increment active process count
	activeCgiProcesses++;
	
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
		
		if (WIFEXITED(status)) {
			int exitCode = WEXITSTATUS(status);
			if (exitCode == 0) {
				process->status = CGI_COMPLETED;
			} else {
				Logger::logMsg(RED, CONSOLE_OUTPUT, "CGI script exited with code %d", exitCode);
				process->status = CGI_ERROR;
			}
		} else if (WIFSIGNALED(status)) {
			int signal = WTERMSIG(status);
			Logger::logMsg(RED, CONSOLE_OUTPUT, "CGI script terminated by signal %d", signal);
			process->status = CGI_ERROR;
		} else {
			Logger::logMsg(RED, CONSOLE_OUTPUT, "CGI script exited abnormally");
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
	// Try different line ending combinations
	size_t headerEndPos = response.find("\r\n\r\n");
	size_t headerEndLength = 4;
	
	if (headerEndPos == std::string::npos) {
		headerEndPos = response.find("\n\n");
		headerEndLength = 2;
	}
	
	std::string httpResponse = "HTTP/1.1 200 OK\r\n";
	std::string cgiHeaders;
	std::string cgiBody;
	
	if (headerEndPos != std::string::npos) {
		cgiHeaders = response.substr(0, headerEndPos);
		cgiBody = response.substr(headerEndPos + headerEndLength);
	} else {
		// No headers found, treat entire response as body
		cgiBody = response;
	}
	
	// Parse CGI headers
	std::istringstream headerStream(cgiHeaders);
	std::string line;
	bool hasContentType = false;
	bool hasStatus = false;
	bool hasContentLength = false;
	bool hasLocation = false;
	
	while (std::getline(headerStream, line)) {
		// Remove \r if present
		if (!line.empty() && line[line.length() - 1] == '\r') {
			line = line.substr(0, line.length() - 1);
		}
		
		// Empty line marks end of headers
		if (line.empty()) {
			break;
		}
		
		// Handle header line continuation (folded headers)
		while (true) {
			std::string nextLine;
			std::streampos pos = headerStream.tellg();
			if (!std::getline(headerStream, nextLine)) {
				break;
			}
			
			// Remove \r if present
			if (!nextLine.empty() && nextLine[nextLine.length() - 1] == '\r') {
				nextLine = nextLine.substr(0, nextLine.length() - 1);
			}
			
			// Check if next line is continuation (starts with space or tab)
			if (!nextLine.empty() && (nextLine[0] == ' ' || nextLine[0] == '\t')) {
				line += " " + nextLine;
			} else {
				// Not a continuation, put back the line
				headerStream.seekg(pos);
				break;
			}
		}
		
		size_t colonPos = line.find(':');
		if (colonPos == std::string::npos) {
			continue; // Skip invalid headers
		}
		
		std::string headerName = line.substr(0, colonPos);
		std::string headerValue = line.substr(colonPos + 1);
		
		// Trim whitespace
		while (!headerValue.empty() && (headerValue[0] == ' ' || headerValue[0] == '\t')) {
			headerValue.erase(0, 1);
		}
		while (!headerValue.empty() && (headerValue[headerValue.length() - 1] == ' ' || headerValue[headerValue.length() - 1] == '\t')) {
			headerValue.erase(headerValue.length() - 1);
		}
		
		// Convert header name to lowercase for comparison
		std::string lowerHeaderName = headerName;
		std::transform(lowerHeaderName.begin(), lowerHeaderName.end(), lowerHeaderName.begin(), ::tolower);
		
		// Check for Status header
		if (lowerHeaderName == "status") {
			// Parse status line - can be "200 OK" or "200" or "200 OK: message"
			std::string statusLine = headerValue;
			size_t firstSpace = statusLine.find(' ');
			if (firstSpace != std::string::npos) {
				// Extract status code
				std::string statusCode = statusLine.substr(0, firstSpace);
				std::string statusText = statusLine.substr(firstSpace + 1);
				
				// Handle case where status text contains a colon
				size_t colonPos = statusText.find(':');
				if (colonPos != std::string::npos) {
					statusText = statusText.substr(0, colonPos);
				}
				
				httpResponse = "HTTP/1.1 " + statusCode + " " + statusText + "\r\n";
			} else {
				// Just a status code
				httpResponse = "HTTP/1.1 " + statusLine + " \r\n";
			}
			hasStatus = true;
		} else if (lowerHeaderName == "content-type") {
			httpResponse += "Content-Type: " + headerValue + "\r\n";
			hasContentType = true;
		} else if (lowerHeaderName == "content-length") {
			httpResponse += "Content-Length: " + headerValue + "\r\n";
			hasContentLength = true;
		} else if (lowerHeaderName == "location") {
			// Handle redirect
			httpResponse += "Location: " + headerValue + "\r\n";
			hasLocation = true;
			if (!hasStatus) {
				// Default to 302 for redirects if no status specified
				httpResponse = "HTTP/1.1 302 Found\r\n" + httpResponse;
				hasStatus = true;
			}
		} else {
			// Handle other headers - preserve original case
			httpResponse += headerName + ": " + headerValue + "\r\n";
		}
	}
	
	// Add default headers if missing
	if (!hasContentType && !hasLocation) {
		httpResponse += "Content-Type: text/html\r\n";
	}
	
	// Add Content-Length only if not already provided by CGI
	if (!hasContentLength) {
		std::ostringstream oss;
		oss << cgiBody.length();
		httpResponse += "Content-Length: " + oss.str() + "\r\n";
	}
	
	// Add standard server headers
	httpResponse += "Server: GetMaBite/1.0\r\n";
	
	// Add connection header for HTTP/1.1
	httpResponse += "Connection: close\r\n";
	
	httpResponse += "\r\n" + cgiBody;
	
	return httpResponse;
}

void CgiHandler::terminateCgi(CgiProcess* process)
{
	if (!process || process->pid <= 0) {
		return;
	}
	
	Logger::logMsg(YELLOW, CONSOLE_OUTPUT, "Terminating CGI process %d", process->pid);
	
	// Send SIGTERM first
	if (kill(process->pid, SIGTERM) == -1) {
		Logger::logMsg(RED, CONSOLE_OUTPUT, "Failed to send SIGTERM to process %d: %s", process->pid, strerror(errno));
	}
	
	// Wait up to 1 second for graceful termination
	for (int i = 0; i < 10; i++) {
		int status;
		pid_t result = waitpid(process->pid, &status, WNOHANG);
		
		if (result > 0) {
			// Process terminated
			Logger::logMsg(GREEN, CONSOLE_OUTPUT, "CGI process %d terminated gracefully", process->pid);
			cleanupProcess(*process);
			return;
		} else if (result == -1) {
			// Error or no such process
			Logger::logMsg(RED, CONSOLE_OUTPUT, "waitpid error for process %d: %s", process->pid, strerror(errno));
			break;
		}
		
		// Wait 100ms before next check
		struct timespec ts;
		ts.tv_sec = 0;
		ts.tv_nsec = 100000000; // 100ms
		nanosleep(&ts, NULL);
	}
	
	// Force kill if still running
	Logger::logMsg(RED, CONSOLE_OUTPUT, "Force killing CGI process %d", process->pid);
	if (kill(process->pid, SIGKILL) == -1) {
		Logger::logMsg(RED, CONSOLE_OUTPUT, "Failed to send SIGKILL to process %d: %s", process->pid, strerror(errno));
	}
	
	// Final cleanup - this should not block
	int status;
	waitpid(process->pid, &status, 0);
	cleanupProcess(*process);
}

void CgiHandler::cleanupProcess(CgiProcess &process)
{
	if (!process.cleaned_up) {
		if (process.stdout_fd != -1) {
			close(process.stdout_fd);
			process.stdout_fd = -1;
		}
		if (process.stdin_fd != -1) {
			close(process.stdin_fd);
			process.stdin_fd = -1;
		}
		process.pid = -1;
		process.cleaned_up = true;
		
		// Decrement active process count
		if (activeCgiProcesses > 0) {
			activeCgiProcesses--;
		}
	}
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
		Logger::logMsg(RED, CONSOLE_OUTPUT, "Directory traversal attempt blocked: %s", path.c_str());
		return false;
	}
	
	// Check for null bytes
	if (path.find('\0') != std::string::npos) {
		Logger::logMsg(RED, CONSOLE_OUTPUT, "Null byte in path blocked: %s", path.c_str());
		return false;
	}
	
	// Check for dangerous characters
	if (path.find(';') != std::string::npos || path.find('|') != std::string::npos || 
	    path.find('&') != std::string::npos || path.find('`') != std::string::npos ||
	    path.find('$') != std::string::npos) {
		Logger::logMsg(RED, CONSOLE_OUTPUT, "Dangerous characters in path blocked: %s", path.c_str());
		return false;
	}
	
	// Check if file exists and is accessible
	struct stat st;
	if (stat(path.c_str(), &st) != 0) {
		Logger::logMsg(RED, CONSOLE_OUTPUT, "CGI file not found: %s (%s)", path.c_str(), strerror(errno));
		return false;
	}
	
	// Check if it's a regular file
	if (!S_ISREG(st.st_mode)) {
		Logger::logMsg(RED, CONSOLE_OUTPUT, "CGI path is not a regular file: %s", path.c_str());
		return false;
	}
	
	// Check if it's readable
	if (access(path.c_str(), R_OK) != 0) {
		Logger::logMsg(RED, CONSOLE_OUTPUT, "CGI file is not readable: %s", path.c_str());
		return false;
	}
	
	// Get the real path to prevent symlink attacks
	char* realPath = realpath(path.c_str(), NULL);
	if (!realPath) {
		Logger::logMsg(RED, CONSOLE_OUTPUT, "Failed to resolve real path for CGI: %s", path.c_str());
		return false;
	}
	
	std::string resolvedPath(realPath);
	free(realPath);
	
	// More flexible web root checking - check for common web directories
	bool inWebRoot = false;
	const char* webRoots[] = {"www/", "public/", "html/", "htdocs/", NULL};
	
	for (int i = 0; webRoots[i] != NULL; i++) {
		if (resolvedPath.find(webRoots[i]) != std::string::npos) {
			inWebRoot = true;
			break;
		}
	}
	
	// Also check if it's in the current working directory tree
	char* cwd = getcwd(NULL, 0);
	if (cwd) {
		std::string cwdStr(cwd);
		free(cwd);
		if (resolvedPath.find(cwdStr) == 0) {
			inWebRoot = true;
		}
	}
	
	if (!inWebRoot) {
		Logger::logMsg(RED, CONSOLE_OUTPUT, "CGI path outside allowed directories: %s", resolvedPath.c_str());
		return false;
	}
	
	return true;
}

std::string CgiHandler::sanitizeHeaderValue(const std::string &value)
{
	std::string sanitized;
	for (size_t i = 0; i < value.length(); ++i) {
		unsigned char c = static_cast<unsigned char>(value[i]);
		
		// Allow printable ASCII characters
		if (c >= 32 && c <= 126) {
			sanitized += c;
		} else if (c == '\t') {
			// Allow tab character
			sanitized += c;
		} else if (c == '\r' || c == '\n') {
			// Convert line endings to space to handle folded headers
			if (i + 1 < value.length()) {
				unsigned char next = static_cast<unsigned char>(value[i + 1]);
				if (next == ' ' || next == '\t') {
					sanitized += ' '; // Convert to space for folded headers
				}
			}
			// Skip the newline character itself
		}
		// Skip other control characters and non-ASCII characters
	}
	
	// Trim trailing whitespace
	while (!sanitized.empty() && (sanitized[sanitized.length() - 1] == ' ' || sanitized[sanitized.length() - 1] == '\t')) {
		sanitized.erase(sanitized.length() - 1);
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
	
	// Extract original request URI from the request line
	std::string requestUri;
	std::istringstream requestStream(_request);
	std::string requestLine;
	if (std::getline(requestStream, requestLine)) {
		std::istringstream lineStream(requestLine);
		std::string method, uri, protocol;
		lineStream >> method >> uri >> protocol;
		requestUri = uri;
	}
	
	// Set SCRIPT_NAME - should be the URL path to the script
	std::string scriptName = requestUri;
	size_t queryPos = scriptName.find('?');
	if (queryPos != std::string::npos) {
		scriptName = scriptName.substr(0, queryPos);
	}
	_cgiEnv["SCRIPT_NAME"] = scriptName;
	
	// Set REQUEST_URI - the original URI from the request
	_cgiEnv["REQUEST_URI"] = requestUri;
	
	// Set SCRIPT_FILENAME - the actual file system path
	_cgiEnv["SCRIPT_FILENAME"] = _cgiPath;
	
	// Server information
	_cgiEnv["SERVER_NAME"] = _serverName;
	_cgiEnv["REMOTE_ADDR"] = _remoteAddr;
	_cgiEnv["HTTP_HOST"] = _serverName;
	_cgiEnv["SERVER_ADMIN"] = "webmaster@" + _serverName;
	
	// Server port
	std::ostringstream portStream;
	portStream << _serverPort;
	_cgiEnv["SERVER_PORT"] = portStream.str();
	
	// Document root - try to determine from script path
	std::string documentRoot = "./www";
	if (_cgiPath.find("www/") != std::string::npos) {
		size_t wwwPos = _cgiPath.find("www/");
		documentRoot = _cgiPath.substr(0, wwwPos + 3); // Include "www"
	}
	_cgiEnv["DOCUMENT_ROOT"] = documentRoot;
	
	// PATH_INFO - extract path info after script name
	std::string pathInfo = "";
	if (!requestUri.empty() && !scriptName.empty()) {
		size_t scriptPos = requestUri.find(scriptName);
		if (scriptPos != std::string::npos) {
			size_t pathInfoStart = scriptPos + scriptName.length();
			size_t queryStart = requestUri.find('?', pathInfoStart);
			if (queryStart != std::string::npos) {
				pathInfo = requestUri.substr(pathInfoStart, queryStart - pathInfoStart);
			} else {
				pathInfo = requestUri.substr(pathInfoStart);
			}
		}
	}
	_cgiEnv["PATH_INFO"] = pathInfo;
	
	// If we have PATH_INFO, set PATH_TRANSLATED
	if (!pathInfo.empty()) {
		_cgiEnv["PATH_TRANSLATED"] = documentRoot + pathInfo;
	}
	
	// Query string
	std::string queryString;
	if (!_cgiParams.empty()) {
		for (std::map<std::string, std::string>::const_iterator it = _cgiParams.begin(); 
		     it != _cgiParams.end(); ++it) {
			if (it != _cgiParams.begin()) {
				queryString += "&";
			}
			queryString += it->first + "=" + it->second;
		}
	} else {
		// Extract query string from REQUEST_URI
		size_t queryPos = requestUri.find('?');
		if (queryPos != std::string::npos) {
			queryString = requestUri.substr(queryPos + 1);
		}
	}
	_cgiEnv["QUERY_STRING"] = queryString;
	
	// Content length and type for POST requests
	if (_method == "POST") {
		std::ostringstream contentLengthStream;
		contentLengthStream << _requestBody.length();
		_cgiEnv["CONTENT_LENGTH"] = contentLengthStream.str();
		
		// Extract Content-Type from request headers
		size_t contentTypePos = _request.find("Content-Type:");
		if (contentTypePos == std::string::npos) {
			contentTypePos = _request.find("content-type:");
		}
		if (contentTypePos != std::string::npos) {
			size_t endPos = _request.find("\r\n", contentTypePos);
			if (endPos == std::string::npos) {
				endPos = _request.find("\n", contentTypePos);
			}
			if (endPos != std::string::npos) {
				std::string contentType = _request.substr(contentTypePos + 13, endPos - (contentTypePos + 13));
				// Trim whitespace
				while (!contentType.empty() && (contentType[0] == ' ' || contentType[0] == '\t')) {
					contentType.erase(0, 1);
				}
				while (!contentType.empty() && (contentType[contentType.length() - 1] == '\r' || contentType[contentType.length() - 1] == '\n')) {
					contentType.erase(contentType.length() - 1);
				}
				_cgiEnv["CONTENT_TYPE"] = sanitizeHeaderValue(contentType);
			}
		}
	} else {
		_cgiEnv["CONTENT_LENGTH"] = "0";
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
	while (std::getline(requestStream, line)) {
		// Remove \r if present
		if (!line.empty() && line[line.length() - 1] == '\r') {
			line = line.substr(0, line.length() - 1);
		}
		
		// Empty line marks end of headers
		if (line.empty()) {
			break;
		}
		
		// Handle header line continuation (folded headers)
		while (true) {
			std::string nextLine;
			std::streampos pos = requestStream.tellg();
			if (!std::getline(requestStream, nextLine)) {
				break;
			}
			
			// Remove \r if present
			if (!nextLine.empty() && nextLine[nextLine.length() - 1] == '\r') {
				nextLine = nextLine.substr(0, nextLine.length() - 1);
			}
			
			// Check if next line is continuation (starts with space or tab)
			if (!nextLine.empty() && (nextLine[0] == ' ' || nextLine[0] == '\t')) {
				line += " " + nextLine;
			} else {
				// Not a continuation, put back the line
				requestStream.seekg(pos);
				break;
			}
		}
		
		size_t colonPos = line.find(':');
		if (colonPos != std::string::npos) {
			std::string headerName = line.substr(0, colonPos);
			std::string headerValue = line.substr(colonPos + 1);
			
			// Trim whitespace from header value
			while (!headerValue.empty() && (headerValue[0] == ' ' || headerValue[0] == '\t')) {
				headerValue.erase(0, 1);
			}
			while (!headerValue.empty() && (headerValue[headerValue.length() - 1] == ' ' || headerValue[headerValue.length() - 1] == '\t')) {
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
			
			// Only add if header name is valid and not a special case
			if (cgiHeaderName.length() > 5) { // More than just "HTTP_"
				// Handle special headers that shouldn't be prefixed with HTTP_
				std::string lowerHeaderName = headerName;
				std::transform(lowerHeaderName.begin(), lowerHeaderName.end(), lowerHeaderName.begin(), ::tolower);
				
				if (lowerHeaderName == "content-type" || lowerHeaderName == "content-length") {
					// These are handled separately in setupEnvironment
					continue;
				}
				
				// Check if we already have this header (case-insensitive)
				bool found = false;
				for (std::map<std::string, std::string>::const_iterator it = _cgiEnv.begin();
				     it != _cgiEnv.end(); ++it) {
					if (it->first == cgiHeaderName) {
						found = true;
						break;
					}
				}
				
				if (!found) {
					_cgiEnv[cgiHeaderName] = headerValue;
				}
			}
		}
	}
}
