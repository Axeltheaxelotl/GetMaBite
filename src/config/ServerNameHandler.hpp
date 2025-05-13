#ifndef SERVERNAMEHANDLER_HPP
#define SERVERNAMEHANDLER_HPP

#include <string>
#include <vector>

class ServerNameHandler {
public:
    // Vérifie si le host correspond à l'un des server_names
    static bool isServerNameMatch(const std::vector<std::string>& serverNames, const std::string& host);
};

#endif // SERVERNAMEHANDLER_HPP
