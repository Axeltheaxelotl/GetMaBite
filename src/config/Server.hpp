#ifndef SERVER_HPP
#define SERVER_HPP

#include<string>
#include<vector>
#include<map>
#include"Location.hpp"

class Server
{
	public:
		std::vector<int> listen_ports;
		std::vector<std::string> server_names;
		std::vector<Location> locations;
		std::string root;
		std::string index;
		std::vector<std::string> allow_methods;

		int client_max_body_size;
		std::map<int, std::string> error_pages;
		std::map<std::string, std::string> cgi_extensions;

		Server();
		~Server();
};

#endif
