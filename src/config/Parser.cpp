/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Parser.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: alanty <alanty@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/14 10:52:37 by smasse            #+#    #+#             */
/*   Updated: 2025/07/12 15:31:19 by alanty           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Parser.hpp"
#include "../utils/Logger.hpp"
#include <iostream>
#include <stdexcept>
#include <cctype>

Parser::Parser() : _currentToken(0) {}

Parser::Parser(const std::string& configFile) : _configFile(configFile), _currentToken(0) {
    parseConfigFile(configFile);
}

Parser::~Parser() {}

void Parser::parseConfigFile(const std::string& configFile) {
    std::ifstream file(configFile.c_str());
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open config file: " + configFile);
    }
    
    std::string content;
    std::string line;
    
    // Lire tout le fichier et nettoyer les commentaires
    while (std::getline(file, line)) {
        skipComments(line);
        if (!trim(line).empty()) {
            content += line + " ";
        }
    }
    file.close();
    
    // Tokenizer le contenu
    tokenize(content);
    
    // Parser les serveurs
    parseServers();
    
    // Valider la configuration
    for (std::vector<Server>::const_iterator it = _servers.begin(); it != _servers.end(); ++it) {
        validateServer(*it);
    }
    
    Logger::logMsg(GREEN, CONSOLE_OUTPUT, "Configuration parsed successfully. Found %zu server(s).", _servers.size());
}

void Parser::tokenize(const std::string& content) {
    _tokens.clear();
    _currentToken = 0;
    
    std::string token;
    for (size_t i = 0; i < content.length(); ++i) {
        char c = content[i];
        
        if (std::isspace(c)) {
            if (!token.empty()) {
                _tokens.push_back(token);
                token.clear();
            }
        } else if (c == '{' || c == '}' || c == ';') {
            if (!token.empty()) {
                _tokens.push_back(token);
                token.clear();
            }
            _tokens.push_back(std::string(1, c));
        } else if (c == '#') {
            // Ignorer les commentaires jusqu'à la fin de la ligne
            while (i < content.length() && content[i] != '\n') {
                ++i;
            }
        } else {
            token += c;
        }
    }
    
    if (!token.empty()) {
        _tokens.push_back(token);
    }
}

void Parser::parseServers() {
    while (hasMoreTokens()) {
        std::string token = getNextToken();
        if (token == "server") {
            expectToken("{");
            Server server;
            parseServer(server);
            expectToken("}");
            _servers.push_back(server);
        } else {
            throw std::runtime_error("Unexpected token: " + token + ". Expected 'server'");
        }
    }
}

void Parser::parseServer(Server& server) {
    while (hasMoreTokens() && peekNextToken() != "}") {
        std::string directive = peekNextToken();
        
        if (directive == "location") {
            getNextToken(); // Consommer le token "location"
            parseLocation(server);
        } else {
            parseDirective(server, NULL);
        }
    }
}

void Parser::parseLocation(Server& server) {
    std::string path = getNextToken();
    expectToken("{");
    
    Location location;
    location.path = path;
    
    while (hasMoreTokens() && peekNextToken() != "}") {
        parseDirective(server, &location);
    }
    
    expectToken("}");
    server.locations.push_back(location);
}

void Parser::parseDirective(Server& server, Location* location) {
    std::string directive = getNextToken();
    
    // Ignorer les points-virgules orphelins
    if (directive == ";") {
        return;
    }
    
    if (directive == "listen") {
        int port = stringToInt(getNextToken());
        if (location) {
            throw std::runtime_error("'listen' directive not allowed in location context");
        }
        server.listen_ports.push_back(port);
        
    } else if (directive == "server_name") {
        if (location) {
            throw std::runtime_error("'server_name' directive not allowed in location context");
        }
        while (hasMoreTokens() && peekNextToken() != ";" && peekNextToken() != "}") {
            server.server_names.push_back(getNextToken());
        }
        
    } else if (directive == "root") {
        std::string rootPath = getNextToken();
        if (location) {
            location->root = rootPath;
        } else {
            server.root = rootPath;
        }
        
    } else if (directive == "index") {
        std::string indexFile = getNextToken();
        if (location) {
            location->index = indexFile;
        } else {
            server.index = indexFile;
        }
        
    } else if (directive == "error_page") {
        int errorCode = stringToInt(getNextToken());
        std::string errorPage = getNextToken();
        if (location) {
            throw std::runtime_error("'error_page' directive not allowed in location context");
        }
        server.error_pages[errorCode] = errorPage;
        
    } else if (directive == "client_max_body_size") {
        size_t maxSize = stringToSize(getNextToken());
        if (location) {
            location->client_max_body_size = maxSize;
        } else {
            server.client_max_body_size = maxSize;
        }
        
    } else if (directive == "autoindex") {
        std::string value = getNextToken();
        bool autoindexValue = (value == "on");
        if (location) {
            location->autoindex = autoindexValue;
        } else {
            server.autoindex = autoindexValue;
        }
        
    } else if (directive == "allow_methods") {
        if (!location) {
            throw std::runtime_error("'allow_methods' directive only allowed in location context");
        }
        while (hasMoreTokens() && peekNextToken() != ";" && peekNextToken() != "}") {
            std::string method = getNextToken();
            if (isValidMethod(method)) {
                location->allow_methods.push_back(method);
            } else {
                throw std::runtime_error("Invalid HTTP method: " + method);
            }
        }
        
    } else if (directive == "return") {
        if (!location) {
            throw std::runtime_error("'return' directive only allowed in location context");
        }
        location->return_code = stringToInt(getNextToken());
        if (hasMoreTokens() && peekNextToken() != ";" && peekNextToken() != "}") {
            location->return_url = getNextToken();
        }
        
    } else if (directive == "alias") {
        if (!location) {
            throw std::runtime_error("'alias' directive only allowed in location context");
        }
        location->alias = getNextToken();
        
    } else if (directive == "upload_path") {
        std::string uploadPath = getNextToken();
        if (location) {
            location->upload_path = uploadPath;
        } else {
            server.upload_path = uploadPath;
        }
        
    } else if (directive == "cgi_extension") {
        std::string extension = getNextToken();
        std::string interpreter = getNextToken();
        if (location) {
            location->cgi_extensions[extension] = interpreter;
        } else {
            server.cgi_extensions[extension] = interpreter;
        }
        
    } else {
        // Ignorer les directives inconnues au lieu de crasher
        Logger::logMsg(YELLOW, CONSOLE_OUTPUT, "Warning: Unknown directive '%s' ignored", directive.c_str());
        // Consommer le token suivant pour éviter les erreurs de parsing
        if (hasMoreTokens() && peekNextToken() != "}" && peekNextToken() != ";") {
            getNextToken();
        }
    }
    
    // Consommer automatiquement le point-virgule suivant s'il existe
    if (hasMoreTokens() && peekNextToken() == ";") {
        getNextToken(); // Consommer le ";"
    }
}

std::string Parser::getNextToken() {
    if (_currentToken >= _tokens.size()) {
        throw std::runtime_error("Unexpected end of file");
    }
    return _tokens[_currentToken++];
}

std::string Parser::peekNextToken() {
    if (_currentToken >= _tokens.size()) {
        return "";
    }
    return _tokens[_currentToken];
}

bool Parser::hasMoreTokens() {
    return _currentToken < _tokens.size();
}

void Parser::expectToken(const std::string& expected) {
    std::string token = getNextToken();
    if (token != expected) {
        throw std::runtime_error("Expected '" + expected + "' but got '" + token + "'");
    }
}

void Parser::skipComments(std::string& line) {
    size_t pos = line.find('#');
    if (pos != std::string::npos) {
        line = line.substr(0, pos);
    }
}

void Parser::validateServer(const Server& server) {
    // Créer une référence non-const pour pouvoir modifier le serveur si nécessaire
    Server& mutableServer = const_cast<Server&>(server);
    
    // Ajouter un port par défaut si aucun n'est spécifié
    if (mutableServer.listen_ports.empty()) {
        mutableServer.listen_ports.push_back(80);
    }
    
    for (std::vector<int>::const_iterator it = mutableServer.listen_ports.begin(); 
         it != mutableServer.listen_ports.end(); ++it) {
        if (!isValidPort(*it)) {
            throw std::runtime_error("Invalid port number");
        }
    }
    
    if (!isValidPath(server.root)) {
        throw std::runtime_error("Invalid root path: " + server.root);
    }
    
    for (std::vector<Location>::const_iterator it = server.locations.begin();
         it != server.locations.end(); ++it) {
        validateLocation(*it);
    }
}

void Parser::validateLocation(const Location& location) {
    if (!isValidPath(location.path)) {
        throw std::runtime_error("Invalid location path: " + location.path);
    }
    
    if (!location.root.empty() && !location.alias.empty()) {
        throw std::runtime_error("Location cannot have both 'root' and 'alias' directives");
    }
}

bool Parser::isValidPath(const std::string& path) {
    if (path.empty()) {
        return false;
    }
    
    // Accepter les chemins absolus (commencent par /)
    if (path[0] == '/') {
        return true;
    }
    
    // Accepter les chemins relatifs (commencent par ./ ou sont des noms simples)
    if (path.length() >= 2 && path.substr(0, 2) == "./") {
        return true;
    }
    
    // Accepter les chemins relatifs simples (sans ./ au début)
    // Vérifier qu'il n'y a pas de caractères dangereux
    if (path.find("..") != std::string::npos) {
        return false; // Rejeter les path traversal
    }
    
    return true;
}

bool Parser::isValidPort(int port) {
    return port > 0 && port <= 65535;
}

bool Parser::isValidMethod(const std::string& method) {
    return method == "GET" || method == "POST" || method == "DELETE" || 
           method == "PUT" || method == "HEAD";
}

const std::vector<Server>& Parser::getServers() const {
    return _servers;
}

std::string Parser::trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return "";
    size_t end = str.find_last_not_of(" \t\n\r");
    return str.substr(start, end - start + 1);
}

std::vector<std::string> Parser::split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::istringstream iss(str);
    std::string token;
    
    while (std::getline(iss, token, delimiter)) {
        token = trim(token);
        if (!token.empty()) {
            tokens.push_back(token);
        }
    }
    
    return tokens;
}

int Parser::stringToInt(const std::string& str) {
    std::istringstream iss(str);
    int value;
    iss >> value;
    if (iss.fail()) {
        throw std::runtime_error("Invalid integer: " + str);
    }
    return value;
}

size_t Parser::stringToSize(const std::string& str) {
    std::istringstream iss(str);
    size_t value;
    iss >> value;
    if (iss.fail()) {
        throw std::runtime_error("Invalid size: " + str);
    }
    return value;
}
