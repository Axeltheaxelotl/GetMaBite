/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   webserv.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: smasse <smasse@student.42luxembourg.lu>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/11 07:19:13 by smasse            #+#    #+#             */
/*   Updated: 2025/03/12 17:26:28 by smasse           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include<iostream>
#include<vector>
#include<string>
#include<dirent.h>
#include<sys/types.h>
#include<map>

bool parser(const std::string& filepath);

class Location
{
	public:
		std::string path;                     // e.g. "/" or "/alias"
		std::string root;                     // root directive
		std::string alias;                    // alias directive (if provided)
		std::vector<std::string> allow_methods; // allow_methods directive
		std::map<std::string, std::string> cgi_extensions; // map: extension -> interpreter
		std::string index;                    // index file
		bool autoindex;                       // autoindex directive
		std::string upload_path;              // upload_path directive
		int return_code;                      // return directive status (e.g. 301)
		std::string return_url;               // return url (if return is used)

		Location() : autoindex(false), return_code(0) {}
};

class Server
{
	public:
		std::vector<int> listen_ports;          // one or more listen directives
		std::vector<std::string> server_names;    // server_name directive(s)
		std::string root;                         // global root directive
		std::string index;                        // global index file
		int client_max_body_size;                 // client_max_body_size directive
		std::map<int, std::string> error_pages;   // error_page mapping: error code -> file
		std::vector<Location> locations;          // location blocks

		Server() : client_max_body_size(0) {}
};