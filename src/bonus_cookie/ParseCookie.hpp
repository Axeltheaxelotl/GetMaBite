#ifndef PARSE_COOKIE_HPP
#define PARSE_COOKIE_HPP

#include <string>
#include <map>

// Utilitaire pour parser l'en-tête Cookie (wrapper simple)
std::map<std::string, std::string> parseCookieHeader(const std::string& cookieHeader);

#endif
