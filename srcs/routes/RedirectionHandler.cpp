#include "RedirectionHandler.hpp"

std::string RedirectionHandler::generateRedirectReponse(int statusCode, const std::string &url)
{
    std::string statusMessage = (statusCode == 301) ? "Moved Permanently" : "Found";
    return "HTTP/1.1 " + std::to_string(statusCode) + " " + statusMessage + "\r\n"
           "Location: " + url + "\r\n\r\n";
}