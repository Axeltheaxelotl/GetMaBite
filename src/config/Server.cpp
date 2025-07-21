/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: alanty <alanty@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/XX/XX XX:XX:XX by alanty           #+#    #+#             */
/*   Updated: 2024/XX/XX XX:XX:XX by alanty          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include"Server.hpp"
#include<algorithm>

Server::Server() : 
    listen_ports(),
    server_names(),
    root(),
    index(),
    error_pages(),
    client_max_body_size(1048576),
    locations(),
    cgi_extensions(),
    autoindex(false),
    allow_methods(),
    upload_path()
{
	// Ne pas ajouter de port par défaut ici - sera fait après le parsing si nécessaire
}

Server::~Server() {}

Location* Server::findLocation(const std::string& path)
{
	Location* bestMatch = NULL;
	size_t bestMatchLength = 0;
	for(std::vector<Location>::iterator it = locations.begin(); it != locations.end(); ++it)
	{
		if(path.find(it->path) == 0)    // Le chemin commence par le chemin de la location
		{
			if(it->path.length() > bestMatchLength)
			{
				bestMatch = &(*it);
				bestMatchLength = it->path.length();
			}
		}
	}
	return bestMatch;
}

const Location* Server::findLocation(const std::string& path) const
{
	const Location* bestMatch = NULL;
	size_t bestMatchLength = 0;
	for(std::vector<Location>::const_iterator it = locations.begin(); it != locations.end(); ++it)
	{
		if(path.find(it->path) == 0)    // Le chemin commence par le chemin de la location
		{
			if(it->path.length() > bestMatchLength)
			{
				bestMatch = &(*it);
				bestMatchLength = it->path.length();
			}
		}
	}
	return bestMatch;
}

std::string Server::getErrorPage(int error_code) const
{
	std::map<int, std::string>::const_iterator it = error_pages.find(error_code);
	if(it != error_pages.end())
	{
		return it->second;
	}
	return ""; // Pas de page d'erreur personnalisée
}

bool Server::isMethodAllowedForPath(const std::string& path, const std::string& method) const
{
	const Location* location = findLocation(path);
	if(location)
	{
		return location->isMethodAllowed(method);
	}
	if(allow_methods.empty())
	{
		return (method == "GET" || method == "POST" || method == "DELETE");
	}
	for(std::vector<std::string>::const_iterator it = allow_methods.begin();
	        it != allow_methods.end(); ++it)
	{
		if(*it == method)
		{
			return true;
		}
	}
	return false;
}

std::string Server::getCgiInterpreterForPath(const std::string& path, const std::string& extension) const
{
	const Location* location = findLocation(path);
	if(location)
	{
		std::string interpreter = location->getCgiInterpreter(extension);
		if(!interpreter.empty())
		{
			return interpreter;
		}
	}
	// Fallback vers les extensions CGI globales du serveur
	std::map<std::string, std::string>::const_iterator it = cgi_extensions.find(extension);
	if(it != cgi_extensions.end())
	{
		return it->second;
	}
	return "";
}
