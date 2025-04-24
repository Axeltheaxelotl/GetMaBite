// Gestion des alias et root

#ifndef ROUTEHANDLER_HPP
#define ROUTEHANDLER_HPP

#include <string>

class RouteHandler
{
    public:
        static std::string resolvePath(const std::string &root, const std::string &alias, const std::string &requestedPath);
};

#endif