#ifndef SERVERROUTER_HPP
#define SERVERROUTER_HPP

#include <string>
#include <vector>
#include "../config/Server.hpp"

class ServerRouter {
public:
    // Retourne l'index du bon bloc Server dans serverConfigs
    static int findServerIndex(const std::vector<Server>& serverConfigs, const std::string& host, int port);
};

#endif
