#ifndef REDIRECTIONHANDLER_HPP
#define REDIRECTIONHANDLER_HPP

#include <string>

class RedirectionHandler
{
    public:
        static std::string generateRedirectReponse(int statusCode, const std::string &url);
};

#endif