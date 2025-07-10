#ifndef CGIHANDLER_HPP
#define CGIHANDLER_HPP

#include<string>
#include<map>
#include<vector>
#include<memory>
#include <unistd.h>
#include <cstring>
#include <sys/wait.h>
#include <cerrno>
#include <signal.h>
#include <time.h>

enum CgiStatus {
	CGI_RUNNING,
	CGI_COMPLETED,
	CGI_ERROR,
	CGI_TIMEOUT
};

struct CgiProcess {
	pid_t pid;
	int stdout_fd;
	int stdin_fd;
	time_t start_time;
	CgiStatus status;
	std::string response;
	bool cleaned_up;
	void* cgiHandler; // Pointer to the CgiHandler that owns this process
	
	CgiProcess() : pid(-1), stdout_fd(-1), stdin_fd(-1), start_time(0), status(CGI_ERROR), cleaned_up(false), cgiHandler(NULL) {}
	
	// Destructor to ensure cleanup
	~CgiProcess() {
		if (!cleaned_up) {
			if (stdout_fd != -1) {
				close(stdout_fd);
			}
			if (stdin_fd != -1) {
				close(stdin_fd);
			}
		}
	}
};

class CgiHandler
{
	private:
		std::string _cgiPath;
		std::string _method;
		std::map<std::string, std::string> _cgiHandler;
		std::map<std::string, std::string> _cgiParams;
		std::map<std::string, std::string> _cgiHeaders;
		std::map<std::string, std::string> _cgiEnv;
		std::string _request; // Stores the complete request
		std::string _requestBody; // Stores just the request body
		std::string _serverName;
		int _serverPort;
		std::string _remoteAddr;
		
		static const int CGI_TIMEOUT_SECONDS = 30; // 30 seconds timeout
		static const int MAX_CGI_PROCESSES = 10; // Limit concurrent CGI processes
		static int activeCgiProcesses; // Track active processes
		
		void setupEnvironment(void);
		void addHttpHeaders(const std::string &request);
		std::vector<char*> buildEnvironment(const std::vector<std::string> &envStrings);
		void cleanupProcess(CgiProcess &process);
		bool validateCgiPath(const std::string &path);
		std::string sanitizeHeaderValue(const std::string &value);

	public:
		CgiHandler(const std::string &cgiPath, const std::string &method,
		           const std::map<std::string, std::string> &cgiHandler,
		           const std::map<std::string, std::string> &cgiParams,
		           const std::string &request, const std::string &serverName,
		           int serverPort, const std::string &remoteAddr);
		
		// Backward compatibility constructor
		CgiHandler(const std::string &cgiPath, const std::string &method,
		           const std::map<std::string, std::string> &cgiHandler,
		           const std::map<std::string, std::string> &cgiParams,
		           const std::string &request);
		
		// Non-blocking CGI execution
		CgiProcess* startCgi();
		CgiStatus checkCgiStatus(CgiProcess* process);
		std::string getCgiResponse(CgiProcess* process);
		void terminateCgi(CgiProcess* process);
		
		// Legacy blocking method (for backward compatibility)
		std::string executeCgi();
		
		// Error handling
		int handleError();
		void setCgiHeaders(const std::map<std::string, std::string> &cgiHeaders);
};

#endif