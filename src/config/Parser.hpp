/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Parser.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: alanty <alanty@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/14 11:20:01 by smasse            #+#    #+#             */
/*   Updated: 2025/07/11 15:11:46 by alanty           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef PARSER_HPP
#define PARSER_HPP

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include "Server.hpp"

class Parser {
private:
    std::vector<Server> _servers;
    std::string _configFile;
    std::vector<std::string> _tokens;
    size_t _currentToken;
    
    // Méthodes de parsing
    void tokenize(const std::string& content);
    void parseServers();
    void parseServer(Server& server);
    void parseLocation(Server& server);
    void parseDirective(Server& server, Location* location = NULL);
    
    // Utilitaires de parsing
    std::string getNextToken();
    std::string peekNextToken();
    bool hasMoreTokens();
    void expectToken(const std::string& expected);
    void skipComments(std::string& line);
    
    // Validation
    void validateServer(const Server& server);
    void validateLocation(const Location& location);
    bool isValidPath(const std::string& path);
    bool isValidPort(int port);
    bool isValidMethod(const std::string& method);

public:
    Parser();
    Parser(const std::string& configFile);
    ~Parser();
    
    void parseConfigFile(const std::string& configFile);
    const std::vector<Server>& getServers() const;
    
    // Méthodes utilitaires
    static std::string trim(const std::string& str);
    static std::vector<std::string> split(const std::string& str, char delimiter);
    static int stringToInt(const std::string& str);
    static size_t stringToSize(const std::string& str);
};

#endif