/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Parser.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: alanty <alanty@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/14 10:52:37 by smasse            #+#    #+#             */
/*   Updated: 2025/06/04 17:45:30 by alanty           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <set>
#include <vector>
#include "Location.hpp"
#include "Server.hpp"

static std::string trim(const std::string &s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

std::vector<Server> parseConfig(const std::string &filepath) {
    std::ifstream file(filepath.c_str());
    if (!file.is_open()) {
        std::cerr << "Error: cannot open config file: " << filepath << std::endl;
        return std::vector<Server>();
    }
    std::vector<Server> servers;
    Server currentServer;
    Location currentLocation;
    bool inServer = false;
    bool inLocation = false;
    std::string line;
    while (std::getline(file, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;
        if (line == "server {") {
            inServer = true;
            currentServer = Server();
            continue;
        }
        if (line.find("location ") == 0 && !line.empty() && line[line.size() - 1] == '{') {
            inLocation = true;
            currentLocation = Location();
            // extract path
            std::istringstream iss(line);
            std::string token, path;
            iss >> token >> path;
            currentLocation.path = trim(path);
            continue;
        }
        if (line == "}") {
            if (inLocation) {
                currentServer.locations.push_back(currentLocation);
                inLocation = false;
            } else if (inServer) {
                // default server_name if none
                if (currentServer.server_names.empty())
                    currentServer.server_names.push_back("_");
                servers.push_back(currentServer);
                inServer = false;
            }
            continue;
        }
        // directive line
        std::istringstream iss(line);
        std::string directive;
        iss >> directive;
        if (directive == "listen") {
            int port;
            iss >> port;
            if (inServer) currentServer.listen_ports.push_back(port);
        } else if (directive == "server_name") {
            std::string name;
            while (iss >> name) {
                if (name.size() > 0 && name[name.size() - 1] == ';') name.erase(name.size() - 1, 1);
                if (inServer) currentServer.server_names.push_back(name);
            }
        } else if (directive == "root") {
            std::string path;
            iss >> path;
            if (path.size() > 0 && path[path.size() - 1] == ';') path.erase(path.size() - 1, 1);
            if (inLocation) currentLocation.root = path;
            else currentServer.root = path;
        } else if (directive == "index") {
            std::string idx;
            iss >> idx;
            if (idx.size() > 0 && idx[idx.size() - 1] == ';') idx.erase(idx.size() - 1, 1);
            if (inLocation) currentLocation.index = idx;
            else currentServer.index = idx;
        } else if (directive == "error_page") {
            int code;
            std::string page;
            iss >> code >> page;
            if (page.size() > 0 && page[page.size() - 1] == ';') page.erase(page.size() - 1, 1);
            currentServer.error_pages[code] = page;
        } else if (directive == "client_max_body_size") {
            int size;
            iss >> size;
            if (inServer) currentServer.client_max_body_size = size;
        } else if (directive == "allow_methods") {
            std::string method;
            while (iss >> method) {
                if (method.size() > 0 && method[method.size() - 1] == ';') method.erase(method.size() - 1, 1);
                if (inLocation) currentLocation.allow_methods.push_back(method);
                else currentServer.allow_methods.push_back(method);
            }
        } else if (directive == "autoindex") {
            std::string val;
            iss >> val;
            bool on = (val == "on" || val == "on;" );
            currentLocation.autoindex = on;
        } else if (directive == "return") {
            int code;
            std::string url;
            iss >> code >> url;
            if (url.size() > 0 && url[url.size() - 1] == ';') url.erase(url.size() - 1, 1);
            currentLocation.return_code = code;
            currentLocation.return_url = url;
        } else if (directive == "upload_path") {
            std::string up;
            iss >> up;
            if (up.size() > 0 && up[up.size() - 1] == ';') up.erase(up.size() - 1, 1);
            currentLocation.upload_path = up;
        } else if (directive == "cgi_extension") {
            std::string ext, handler;
            iss >> ext >> handler;
            if (handler.size() > 0 && handler[handler.size() - 1] == ';') handler.erase(handler.size() - 1, 1);
            currentLocation.cgi_extensions[ext] = handler;
        }
        // ignore other directives for now
    }
    return servers;
}
