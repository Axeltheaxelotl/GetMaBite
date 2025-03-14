#include"Runner.hpp"
#include"../parser/Server.hpp"
#include"../parser/Parser.hpp"
#include"Runner.hpp"
#include<iostream>
#include<unistd.h>
#include<string>
#include<fstream>
#include<cstring>
#include<vector>
#include<sstream>
#include<sys/socket.h>
#include<arpa/inet.h>

static void sendResponse(int client_fd, const std::string &response)
{
	send(client_fd, response.c_str(), response.size(), 0);
	close(client_fd);
}

static void handleGET(int client_fd, const std::string &indexPath, int ret_code, const std::string &ret_url)
{
	if(ret_code != 0 && !ret_url.empty())
	{
		std::ostringstream oss;
		oss << ret_code;
		std::string response = "HTTP/1.1 " + oss.str() + " Moved\r\nLocation: " + ret_url + "\r\n\r\n";
		sendResponse(client_fd, response);
		return;
	}
	std::ifstream file(indexPath.c_str());
	if(file)
	{
		std::string content((std::istreambuf_iterator<char>(file)),
		                    std::istreambuf_iterator<char>());
		std::string response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n" + content;
		sendResponse(client_fd, response);
	}
	else
	{
		std::string response = "HTTP/1.1 404 Not Found\r\n\r\nFile not found.";
		sendResponse(client_fd, response);
	}
}

static void handlePOST(int client_fd)
{
	std::string response = "HTTP/1.1 200 OK\r\n\r\nPOST request received.";
	sendResponse(client_fd, response);
}

static void handleDELETE(int client_fd)
{
	std::string response = "HTTP/1.1 200 OK\r\n\r\nDELETE request received.";
	sendResponse(client_fd, response);
}

static void handleRequest(int client_fd, const std::string &indexPath, const std::vector<Location>& locations)
{
	char buffer[1024] = {0};
	int valread = read(client_fd, buffer, sizeof(buffer));
	if(valread <= 0)
	{
		close(client_fd);
		return;
	}
	std::string request(buffer);
	int ret_code = 0;
	std::string ret_url = "";
	if(request.find("GET") != std::string::npos)
	{
		std::istringstream iss(request);
		std::string method, url, version;
		iss >> method >> url >> version;
		for(size_t i = 0; i < locations.size(); i++)
		{
			if(url.find(locations[i].path) == 0)
			{
				ret_code = locations[i].return_code;
				ret_url = locations[i].return_url;
				break;
			}
		}
		handleGET(client_fd, indexPath, ret_code, ret_url);
	}
	else if(request.find("POST") != std::string::npos)
		handlePOST(client_fd);
	else if(request.find("DELETE") != std::string::npos)
		handleDELETE(client_fd);
	else
	{
		std::string response = "HTTP/1.1 405 Method Not Allowed\r\n\r\n";
		sendResponse(client_fd, response);
	}
}

Runner::Runner()
{
}

Runner::~Runner()
{
}

void Runner::run(int argc, char **argv)
{
	std::string configPath = "config.conf";
	if(argc > 1)
		configPath = argv[1];
	std::vector<Server> servers = parseConfig(configPath);
	if(servers.empty())
	{
		std::cerr << "Failed to load configuration." << std::endl;
		return;
	}
	Server config = servers[0];
	if(config.listen_ports.empty())
	{
		std::cerr << "No port specified in the configuration." << std::endl;
		return;
	}
	int port = config.listen_ports[0];
	std::string indexPath = config.index.empty() ? "./index.html" : config.index;
	int server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(server_fd < 0)
	{
		std::cerr << "Socket creation error" << std::endl;
		return;
	}
	struct sockaddr_in address;
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(port);
	if(bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
	{
		std::cerr << "Bind failed" << std::endl;
		return;
	}
	if(listen(server_fd, 3) < 0)
	{
		std::cerr << "Listen failed" << std::endl;
		return;
	}
	std::cout << "Server configuration:" << std::endl;
	std::cout << "Port: " << port << std::endl;
	std::cout << "Index path: " << indexPath << std::endl;
	std::cout << "Server running on port " << port << "..." << std::endl;
	while(true)
	{
		struct sockaddr_in client_address;
		socklen_t addrlen = sizeof(client_address);
		int client_fd = accept(server_fd, (struct sockaddr *)&client_address, &addrlen);
		if(client_fd < 0)
		{
			std::cerr << "Accept failed" << std::endl;
			continue;
		}
		std::cout << "New connection from " << inet_ntoa(client_address.sin_addr)
		          << ":" << ntohs(client_address.sin_port) << std::endl;
		handleRequest(client_fd, indexPath, config.locations);
	}
}