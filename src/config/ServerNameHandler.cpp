#include "ServerNameHandler.hpp"
#include <algorithm>

bool ServerNameHandler::isServerNameMatch(const std::vector<std::string>& serverNames, const std::string& host) {
    return std::find(serverNames.begin(), serverNames.end(), host) != serverNames.end();
}
/*
 * 
 * 
*/

