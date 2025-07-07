/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Parser.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: smasse <smasse@student.42luxembourg.lu>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/14 10:52:37 by smasse            #+#    #+#             */
/*   Updated: 2025/06/12 18:31:00 by smasse           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include<algorithm>
#include<cctype>
#include<fstream>
#include<set>
#include<iostream>
#include<sstream>
#include"Server.hpp"      // added new include
#include"Location.hpp"    // added new include

struct NotSpace
{
	bool operator()(int ch) const
	{
		return (!std::isspace(ch));
	}
};

static std::string trim(const std::string &s)
{
	std::string result = s;
	result.erase(result.begin(), std::find_if(result.begin(), result.end(), NotSpace()));
	result.erase(std::find_if(result.rbegin(), result.rend(), NotSpace()).base(), result.end());
	return (result);
}

// New helper to resolve relative paths based on a given base.
static std::string resolvePath(const std::string &base, const std::string &path)
{
	if(path.empty() || path[0] == '/')
		return path;
	if(base.empty())
		return path;
	return base + "/" + path;
}

static bool isValidIp(const std::string &ip)
{
	int segments;
	int num;
	std::istringstream iss(ip);
	std::string octet;
	segments = 0;
	while(std::getline(iss, octet, '.'))
	{
		if(octet.empty() || octet.length() > 3)
			return (false);
		for(size_t i = 0; i < octet.length(); ++i)
			if(!std::isdigit(octet[i]))
				return (false);
		num = std::atoi(octet.c_str());
		if(num < 0 || num > 255)
			return (false);
		++segments;
	}
	return (segments == 4);
}

static bool isAllDigits(const std::string &s)
{
	for(std::string::const_iterator it = s.begin(); it != s.end(); ++it)
	{
		if(!std::isdigit(*it))
			return (false);
	}
	return (true);
}

static bool validateServers(const std::vector<Server> &servers)
{
	for(std::vector<Server>::const_iterator it = servers.begin(); it != servers.end(); ++it)
	{
		std::set<std::string> uniqueNames;
		for(std::vector<std::string>::const_iterator nameIt = it->server_names.begin(); nameIt != it->server_names.end(); ++nameIt)
		{
			if(!uniqueNames.insert(*nameIt).second)
			{
				std::cerr << "Error: duplicate server name in one block: " << *nameIt << std::endl;
				return (false);
			}
		}
		std::set<std::string> uniquePaths;
		for(std::vector<Location>::const_iterator locIt = it->locations.begin(); locIt != it->locations.end(); ++locIt)
		{
			if(!uniquePaths.insert(locIt->path).second)
			{
				std::cerr << "Error: duplicate location path: " << locIt->path << std::endl;
				return (false);
			}
			if(!locIt->root.empty() && !locIt->alias.empty())
			{
				std::cerr << "Error: location cannot have both root and alias: " << locIt->path << std::endl;
				return (false);
			}
		}
	}
	std::set<std::string> globalNames;
	for(std::vector<Server>::const_iterator it = servers.begin(); it != servers.end(); ++it)
	{
		for(std::vector<std::string>::const_iterator nameIt = it->server_names.begin(); nameIt != it->server_names.end(); ++nameIt)
		{
			if(!globalNames.insert(*nameIt).second)
			{
				std::cerr << "Error: duplicate server name across servers: " << *nameIt << std::endl;
				return (false);
			}
		}
	}
	return (true);
}

std::vector<Server> parseConfig(const std::string &filepath)
{
	Server *currentServer;
	Location *currentLocation;
	int port;
	size_t colonPos;
	int size;
	int code;
	int status;
	std::ifstream file(filepath.c_str());
	if(!file.is_open())
	{
		std::cerr << "Error: cannot open file: " << filepath << std::endl;
		return std::vector<Server>(); // error return
	}
	std::vector<Server> servers;
	currentServer = 0;
	currentLocation = 0;
	std::string line;
	while(std::getline(file, line))
	{
		line = trim(line);
		if(line.empty() || line[0] == '#')
			continue ;
		if(line.find("server") == 0 && line.find("{") != std::string::npos)
		{
			servers.push_back(Server());
			currentServer = &servers.back();
			continue ;
		}
		if(line.find("location") == 0 && line.find("{") != std::string::npos)
		{
			std::istringstream iss(line);
			std::string keyword, path;
			iss >> keyword >> path;
			if(currentServer)
			{
				currentServer->locations.push_back(Location());
				currentLocation = &currentServer->locations.back();
				currentLocation->path = path;
			}
			continue ;
		}
		if(line == "}")
		{
			if(currentLocation)
				currentLocation = 0;
			else
				currentServer = 0;
			continue ;
		}
		std::istringstream iss(line);
		std::string directive;
		iss >> directive;
		if(directive == "listen")
		{
			if(currentLocation)
			{
				std::cerr << "Error: 'listen' directive is only allowed in server blocks." << std::endl;
				return std::vector<Server>(); // error return
			}
			std::string param;
			iss >> param;

			if (!param.empty() && param[param.size() - 1] == ';')
    			param.erase(param.size() - 1);

			port = 0;
			colonPos = param.find(':');
			if(colonPos != std::string::npos)
			{
				std::string ipPart = param.substr(0, colonPos);
				std::string portPart = param.substr(colonPos + 1);
				if(!isValidIp(ipPart))
				{
					std::cerr << "Error: invalid IP: " << ipPart << std::endl;
					return std::vector<Server>(); // error return
				}
				if(portPart.empty() || !isAllDigits(portPart))
				{
					std::cerr << "Error: invalid port value: " << portPart << std::endl;
					return std::vector<Server>(); // error return
				}
				port = std::atoi(portPart.c_str());
			}
			else
			{
				if(param.empty() || !isAllDigits(param))
				{
					std::cerr << "Error: invalid port value: " << param << std::endl;
					return std::vector<Server>(); // error return
				}
				port = std::atoi(param.c_str());
			}
			if(port < 1 || port > 65535)
			{
				std::cerr << "Error: invalid port: " << port << std::endl;
				return std::vector<Server>(); // error return
			}
			if(currentServer)
				currentServer->listen_ports.push_back(port);
		}
		else if(directive == "server_name")
		{
			if(currentLocation)
			{
				std::cerr << "Error: 'server_name' directive is not allowed inside location blocks." << std::endl;
				return std::vector<Server>(); // error return
			}
			std::string name;
			iss >> name;
			if(currentServer)
				currentServer->server_names.push_back(name);
		}
		else if(directive == "client_max_body_size")
		{
			if(currentLocation)
			{
				std::cerr << "Error: 'client_max_body_size' directive is only allowed in server blocks." << std::endl;
				return std::vector<Server>(); // error return
			}
			iss >> size;
			if(currentServer)
				currentServer->client_max_body_size = size;
		}
		else if(directive == "error_page")
		{
			if(currentLocation)
			{
				std::cerr << "Error: 'error_page' directive is only allowed in server blocks." << std::endl;
				return std::vector<Server>(); // error return
			}
			std::string page;
			iss >> code >> page;
			if(currentServer)
				currentServer->error_pages[code] = page;
		}
		else if(directive == "root")
		{
			std::string path;
			iss >> path;
			if(currentLocation)
			{
				if(!currentLocation->root.empty())
				{
					std::cerr << "Error: duplicate root directive in location block." << std::endl;
					return std::vector<Server>(); // error return
				}
				currentLocation->root = path;
			}
			else if(currentServer)
			{
				if(!currentServer->root.empty())
				{
					std::cerr << "Error: duplicate root directive in server block." << std::endl;
					return std::vector<Server>(); // error return
				}
				currentServer->root = path;
			}
		}
		else if(directive == "index")
		{
			std::string idx;
			iss >> idx;
			if(currentLocation)
			{
				currentLocation->index = idx;
			}
			else if(currentServer)
			{
				currentServer->index = idx;
			}
		}
		else if(directive == "allow_methods")
		{
			std::string method;
			while(iss >> method)
			{
				if(currentLocation)
					currentLocation->allow_methods.push_back(method);
									else if(currentServer)
					currentServer->allow_methods.push_back(method);
			}
		}
		else if(directive == "cgi_extension")
		{
			std::string extension, interpreter;
			iss >> extension >> interpreter;
			if(currentLocation)
				currentLocation->cgi_extensions[extension] = interpreter;
							else if(currentServer)
				currentServer->cgi_extensions[extension] = interpreter;
		}
		else if(directive == "upload_path")
		{
			std::string path;
			iss >> path;
			if(currentLocation)
			{
				currentLocation->upload_path = resolvePath((!currentLocation->root.empty() ? currentLocation->root : (currentServer ? currentServer->root : "")), path);
			}
		}
		else if(directive == "autoindex")
		{
			std::string value;
			iss >> value;
			if(currentLocation)
				currentLocation->autoindex = (value == "on");
		}
		else if(directive == "return")
		{
			std::string url;
			iss >> status >> url;
			if(currentLocation)
			{
				currentLocation->return_code = status;
				currentLocation->return_url = url;
			}
		}
		else if(directive == "alias")
		{
			std::string alias;
			iss >> alias;
			if(currentLocation)
			{
				currentLocation->alias = resolvePath((!currentLocation->root.empty() ? currentLocation->root : (currentServer ? currentServer->root : "")), alias);
			}
			else
			{
				std::cerr << "Error: 'alias' directive is only allowed inside location blocks." << std::endl;
				return std::vector<Server>(); // error return
			}
		}
		else
		{
			std::cerr << "Error: unknown directive: " << directive << std::endl;
			return std::vector<Server>(); // error return
		}
	}
	if(!validateServers(servers))
		return std::vector<Server>(); // error return
	return servers;
}
