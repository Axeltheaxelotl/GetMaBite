#include "RouteHandler.hpp"

std::string resolvePath(const std::string &root, const std::string &alias, const std::string &requestedPath)
{
    if (!alias.empty())
        return alias + requestedPath.substr(1); // ca supprime le premier / de requestedPath
    return root + requestedPath;
}