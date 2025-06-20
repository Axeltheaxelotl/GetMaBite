#ifndef CGIHANDLER_HPP
#define CGIHANDLER_HPP

#include<string>
#include<map>
#include <unistd.h>
#include <cstring>
#include <sys/wait.h>
#include <cerrno>

class CgiHandler
{
	private:
		int _client_fd;
		std::string _cgiPath;
		std::string _method;
		std::map<std::string, std::string> _cgiHandler;
		std::map<std::string, std::string> _cgiParams;
		std::map<std::string, std::string> _cgiHeaders;
		std::map<std::string, std::string> _cgiEnv;
		std::string _request; // Stores the request body
		int _pipe_out[2];
		int _pipe_in[2];
		pid_t _pid;

	public:
		CgiHandler(int client_fd, const std::string &cgiPath, const std::string &method,
		           const std::map<std::string, std::string> &cgiHandler,
		           const std::map<std::string, std::string> &cgiParams,
		           const std::string &request);
		void handleErrorCGI(int client_fd);
		void setCgiHeaders(const std::map<std::string, std::string> &cgiHeaders);
		void setCgiEnv(void);
		std::string executeCgi();
		void forkProcess();
		int getOutputFd() const;
		pid_t getPid() const;
		int getClientFd() const;
};

#endif