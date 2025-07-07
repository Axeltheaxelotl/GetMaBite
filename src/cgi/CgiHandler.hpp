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

	public:
		CgiHandler(int client_fd, const std::string &cgiPath, const std::string &method,
		           const std::map<std::string, std::string> &cgiHandler,
		           const std::map<std::string, std::string> &cgiParams,
		           const std::string &request);
		void handleError(int client_fd);
		void setCgiHeaders(const std::map<std::string, std::string> &cgiHeaders);
		void setCgiEnv(void);
		std::string executeCgi();
};

#endif