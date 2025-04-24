// Gestion des redirection HTPP

#include "RedirectionHandler.hpp"
#include <sstream>

static std::string to_string(int value)
{
    std::ostringstream oss;
    oss << value;
    return oss.str();
}

std::string RedirectionHandler::generateRedirectReponse(int statusCode, const std::string &url)
{
    std::string statusMessage = (statusCode == 301) ? "Moved Permanently" : "Found";
    return "HTTP/1.1 " + to_string(statusCode) + " " + statusMessage + "\r\n"
           "Location: " + url + "\r\n\r\n";
}