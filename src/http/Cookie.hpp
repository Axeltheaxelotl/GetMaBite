#ifndef COOKIE_HPP
#define COOKIE_HPP

#include <string>
#include <map>
#include <vector>
#include <ctime>

class Cookie {
private:
    std::string _name;
    std::string _value;
    std::string _domain;
    std::string _path;
    time_t _expires;
    bool _httpOnly;
    bool _secure;
    int _maxAge;

public:
    Cookie();
    Cookie(const std::string& name, const std::string& value);
    Cookie(const std::string& name, const std::string& value, const std::string& path);
    ~Cookie();

    // Getters
    const std::string& getName() const;
    const std::string& getValue() const;
    const std::string& getDomain() const;
    const std::string& getPath() const;
    time_t getExpires() const;
    bool isHttpOnly() const;
    bool isSecure() const;
    int getMaxAge() const;

    // Setters
    void setName(const std::string& name);
    void setValue(const std::string& value);
    void setDomain(const std::string& domain);
    void setPath(const std::string& path);
    void setExpires(time_t expires);
    void setHttpOnly(bool httpOnly);
    void setSecure(bool secure);
    void setMaxAge(int maxAge);

    // Utility methods
    std::string toString() const;  // Generate Set-Cookie header string
    bool isExpired() const;
    bool isValid() const;
    
    // Static utility methods
    static std::string formatTime(time_t time);
    static time_t parseTime(const std::string& timeStr);
};

class CookieManager {
private:
    std::map<std::string, Cookie> _cookies;  // name -> cookie
    
public:
    CookieManager();
    ~CookieManager();
    
    // Cookie management
    void addCookie(const Cookie& cookie);
    void removeCookie(const std::string& name);
    Cookie* getCookie(const std::string& name);
    const Cookie* getCookie(const std::string& name) const;
    bool hasCookie(const std::string& name) const;
    void clearExpiredCookies();
    void clearAllCookies();
    
    // Parsing and generation
    void parseCookieHeader(const std::string& cookieHeader);
    std::vector<std::string> generateSetCookieHeaders() const;
    std::string generateCookieHeader() const;
    
    // Utility methods
    size_t getCookieCount() const;
    std::vector<std::string> getCookieNames() const;
};

// Session management helper class
class SessionManager {
private:
    static std::map<std::string, std::map<std::string, std::string> > _sessions;
    static std::map<std::string, time_t> _sessionExpires;
    static int _sessionTimeout;
    
    static std::string generateSessionId();
    
public:
    static void setSessionTimeout(int timeoutSeconds);
    static std::string createSession();
    static bool isValidSession(const std::string& sessionId);
    static void destroySession(const std::string& sessionId);
    static void cleanupExpiredSessions();
    
    // Session data management
    static void setSessionData(const std::string& sessionId, const std::string& key, const std::string& value);
    static std::string getSessionData(const std::string& sessionId, const std::string& key);
    static bool hasSessionData(const std::string& sessionId, const std::string& key);
    static void removeSessionData(const std::string& sessionId, const std::string& key);
    
    // Statistics
    static size_t getActiveSessionCount();
};

#endif
