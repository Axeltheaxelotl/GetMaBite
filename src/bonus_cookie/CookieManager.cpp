#include "CookieManager.hpp"
#include <sstream>

std::map<std::string, std::string> CookieManager::parseCookies(const std::string& cookieHeader) {
    std::map<std::string, std::string> cookies;
    std::istringstream stream(cookieHeader);
    std::string pair;
    while (std::getline(stream, pair, ';')) {
        size_t eq = pair.find('=');
        if (eq != std::string::npos) {
            std::string name = pair.substr(0, eq);
            std::string value = pair.substr(eq + 1);
            // Trim spaces
            while (!name.empty() && (name[0] == ' ' || name[0] == '\t')) name.erase(0, 1);
            while (!value.empty() && (value[0] == ' ' || value[0] == '\t')) value.erase(0, 1);
            cookies[name] = value;
        }
    }
    return cookies;
}

std::string CookieManager::createSetCookieHeader(const std::string& name, const std::string& value, int maxAge, const std::string& path) {
    std::ostringstream oss;
    oss << "Set-Cookie: " << name << "=" << value << "; Path=" << path;
    if (maxAge >= 0)
        oss << "; Max-Age=" << maxAge;
    return oss.str();
}
