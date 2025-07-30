/* ************************************************************************** */
/*                              // Keep the pattern with the highest "specificity" score
        if (doesMatch && specificity > bestSpecificity) {
            bestMatch       = &(*it);
            bestSpecificity = specificity;
        }                                                 */
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
#include <fnmatch.h>     // for fnmatch

Server::Server() : 
    listen_ports(),
    server_names(),
    root(),
    index(),
    error_pages(),
    client_max_body_size(0),
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
		const std::string& locationPath = it->path;
		
		// Check if this location matches the requested path
		bool matches = false;
		
		// Handle exact match or prefix match with proper boundary checking
		if (path == locationPath) {
			matches = true;
		} else if (path.length() > locationPath.length() && 
		           path.compare(0, locationPath.length(), locationPath) == 0) {
			// Ensure we match at path boundaries
			if (locationPath == "/" || path[locationPath.length()] == '/') {
				matches = true;
			}
		}
		
		if (matches && locationPath.length() > bestMatchLength) {
			bestMatch = &(*it);
			bestMatchLength = locationPath.length();
		}
	}
	return bestMatch;
}

const Location* Server::findLocation(const std::string& path) const
{
    const Location* bestMatch      = NULL;
    size_t          bestSpecificity = 0;

    for (std::vector<Location>::const_iterator it = locations.begin();
         it != locations.end(); ++it)
    {
        const std::string& pattern = it->path;
        bool doesMatch = false;
        size_t specificity = 0;

        // If the location contains '*' or '?' use fnmatch()
        if (pattern.find_first_of("*?") != std::string::npos) {
            if (fnmatch(pattern.c_str(), path.c_str(), 0) == 0) {
                doesMatch = true;
                // Reward non‑wildcard characters
                specificity = pattern.length()
                              - std::count(pattern.begin(), pattern.end(), '*')
                              - std::count(pattern.begin(), pattern.end(), '?');
            }
        }
        // Otherwise fall back to simple prefix match with proper boundary checking
        else {
            // Handle exact match
            if (path == pattern) {
                doesMatch = true;
                specificity = pattern.length();
            } 
            // Handle prefix match with proper boundary checking
            else if (path.length() > pattern.length() && 
                     path.compare(0, pattern.length(), pattern) == 0) {
                // Ensure we match at path boundaries
                if (pattern == "/" || path[pattern.length()] == '/') {
                    doesMatch = true;
                    specificity = pattern.length();
                }
            }
        }

        // Keep the pattern with the highest “specificity” score
        if (doesMatch && specificity > bestSpecificity) {
            bestMatch       = &(*it);
            bestSpecificity = specificity;
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
