#ifndef COOKIE_MANAGER_HPP
#define COOKIE_MANAGER_HPP

#include <string>
#include <map>
#include <vector>

class CookieManager {
public:
    // Parse l'en-tête Cookie d'une requête HTTP
    static std::map<std::string, std::string> parseCookies(const std::string& cookieHeader);
    // Génère un en-tête Set-Cookie pour la réponse HTTP
    static std::string createSetCookieHeader(const std::string& name, const std::string& value, int maxAge = -1, const std::string& path = "/");
};

#endif // COOKIE_MANAGER_HPP
