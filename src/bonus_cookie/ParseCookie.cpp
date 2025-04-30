#include "ParseCookie.hpp"
#include "CookieManager.hpp"

std::map<std::string, std::string> parseCookieHeader(const std::string& cookieHeader) {
    return CookieManager::parseCookies(cookieHeader);
}
