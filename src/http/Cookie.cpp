#include "Cookie.hpp"
#include <sstream>
#include <cstdlib>
#include <algorithm>
#include <iomanip>

// Initialize static members
std::map<std::string, std::map<std::string, std::string> > SessionManager::_sessions;
std::map<std::string, time_t> SessionManager::_sessionExpires;
int SessionManager::_sessionTimeout = 3600; // 1 hour default

// Cookie implementation
Cookie::Cookie() : _name(""), _value(""), _domain(""), _path("/"), 
                   _expires(0), _httpOnly(false), _secure(false), _maxAge(-1) {}

Cookie::Cookie(const std::string& name, const std::string& value) 
    : _name(name), _value(value), _domain(""), _path("/"), 
      _expires(0), _httpOnly(false), _secure(false), _maxAge(-1) {}

Cookie::Cookie(const std::string& name, const std::string& value, const std::string& path)
    : _name(name), _value(value), _domain(""), _path(path), 
      _expires(0), _httpOnly(false), _secure(false), _maxAge(-1) {}

Cookie::~Cookie() {}

// Getters
const std::string& Cookie::getName() const { return _name; }
const std::string& Cookie::getValue() const { return _value; }
const std::string& Cookie::getDomain() const { return _domain; }
const std::string& Cookie::getPath() const { return _path; }
time_t Cookie::getExpires() const { return _expires; }
bool Cookie::isHttpOnly() const { return _httpOnly; }
bool Cookie::isSecure() const { return _secure; }
int Cookie::getMaxAge() const { return _maxAge; }

// Setters
void Cookie::setName(const std::string& name) { _name = name; }
void Cookie::setValue(const std::string& value) { _value = value; }
void Cookie::setDomain(const std::string& domain) { _domain = domain; }
void Cookie::setPath(const std::string& path) { _path = path; }
void Cookie::setExpires(time_t expires) { _expires = expires; }
void Cookie::setHttpOnly(bool httpOnly) { _httpOnly = httpOnly; }
void Cookie::setSecure(bool secure) { _secure = secure; }
void Cookie::setMaxAge(int maxAge) { _maxAge = maxAge; }

std::string Cookie::toString() const {
    std::ostringstream oss;
    oss << _name << "=" << _value;
    
    if (!_path.empty() && _path != "/") {
        oss << "; Path=" << _path;
    }
    
    if (!_domain.empty()) {
        oss << "; Domain=" << _domain;
    }
    
    if (_maxAge >= 0) {
        oss << "; Max-Age=" << _maxAge;
    } else if (_expires > 0) {
        oss << "; Expires=" << formatTime(_expires);
    }
    
    if (_httpOnly) {
        oss << "; HttpOnly";
    }
    
    if (_secure) {
        oss << "; Secure";
    }
    
    return oss.str();
}

bool Cookie::isExpired() const {
    if (_maxAge >= 0) {
        return false; // Max-Age is handled by browser
    }
    if (_expires > 0) {
        return time(NULL) > _expires;
    }
    return false; // Session cookie
}

bool Cookie::isValid() const {
    return !_name.empty() && !isExpired();
}

std::string Cookie::formatTime(time_t time) {
    struct tm* gmt = gmtime(&time);
    std::ostringstream oss;
    
    // Format: Wdy, DD Mon YYYY HH:MM:SS GMT
    const char* days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    const char* months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                           "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    
    oss << days[gmt->tm_wday] << ", "
        << std::setfill('0') << std::setw(2) << gmt->tm_mday << " "
        << months[gmt->tm_mon] << " "
        << (gmt->tm_year + 1900) << " "
        << std::setw(2) << gmt->tm_hour << ":"
        << std::setw(2) << gmt->tm_min << ":"
        << std::setw(2) << gmt->tm_sec << " GMT";
    
    return oss.str();
}

time_t Cookie::parseTime(const std::string& timeStr) {
    // Simple implementation - in production use strptime
    // For now, return current time + 1 hour as fallback
    return time(NULL) + 3600;
}

// CookieManager implementation
CookieManager::CookieManager() {}
CookieManager::~CookieManager() {}

void CookieManager::addCookie(const Cookie& cookie) {
    if (cookie.isValid()) {
        _cookies[cookie.getName()] = cookie;
    }
}

void CookieManager::removeCookie(const std::string& name) {
    _cookies.erase(name);
}

Cookie* CookieManager::getCookie(const std::string& name) {
    std::map<std::string, Cookie>::iterator it = _cookies.find(name);
    return (it != _cookies.end()) ? &it->second : NULL;
}

const Cookie* CookieManager::getCookie(const std::string& name) const {
    std::map<std::string, Cookie>::const_iterator it = _cookies.find(name);
    return (it != _cookies.end()) ? &it->second : NULL;
}

bool CookieManager::hasCookie(const std::string& name) const {
    return _cookies.find(name) != _cookies.end();
}

void CookieManager::clearExpiredCookies() {
    std::map<std::string, Cookie>::iterator it = _cookies.begin();
    while (it != _cookies.end()) {
        if (it->second.isExpired()) {
            _cookies.erase(it++);
        } else {
            ++it;
        }
    }
}

void CookieManager::clearAllCookies() {
    _cookies.clear();
}

void CookieManager::parseCookieHeader(const std::string& cookieHeader) {
    if (cookieHeader.empty()) return;
    
    std::istringstream iss(cookieHeader);
    std::string pair;
    
    // Parse "name1=value1; name2=value2; ..."
    while (std::getline(iss, pair, ';')) {
        // Trim whitespace
        size_t start = pair.find_first_not_of(" \t");
        if (start == std::string::npos) continue;
        size_t end = pair.find_last_not_of(" \t");
        pair = pair.substr(start, end - start + 1);
        
        // Find '=' separator
        size_t eq = pair.find('=');
        if (eq != std::string::npos && eq > 0) {
            std::string name = pair.substr(0, eq);
            std::string value = pair.substr(eq + 1);
            
            Cookie cookie(name, value);
            addCookie(cookie);
        }
    }
}

std::vector<std::string> CookieManager::generateSetCookieHeaders() const {
    std::vector<std::string> headers;
    
    for (std::map<std::string, Cookie>::const_iterator it = _cookies.begin();
         it != _cookies.end(); ++it) {
        if (it->second.isValid()) {
            headers.push_back(it->second.toString());
        }
    }
    
    return headers;
}

std::string CookieManager::generateCookieHeader() const {
    std::ostringstream oss;
    bool first = true;
    
    for (std::map<std::string, Cookie>::const_iterator it = _cookies.begin();
         it != _cookies.end(); ++it) {
        if (it->second.isValid()) {
            if (!first) oss << "; ";
            oss << it->second.getName() << "=" << it->second.getValue();
            first = false;
        }
    }
    
    return oss.str();
}

size_t CookieManager::getCookieCount() const {
    return _cookies.size();
}

std::vector<std::string> CookieManager::getCookieNames() const {
    std::vector<std::string> names;
    for (std::map<std::string, Cookie>::const_iterator it = _cookies.begin();
         it != _cookies.end(); ++it) {
        names.push_back(it->first);
    }
    return names;
}

// SessionManager implementation
void SessionManager::setSessionTimeout(int timeoutSeconds) {
    _sessionTimeout = timeoutSeconds;
}

std::string SessionManager::generateSessionId() {
    std::ostringstream oss;
    srand(time(NULL));
    
    // Generate random session ID
    for (int i = 0; i < 32; ++i) {
        int r = rand() % 36;
        if (r < 10) {
            oss << char('0' + r);
        } else {
            oss << char('a' + r - 10);
        }
    }
    
    return oss.str();
}

std::string SessionManager::createSession() {
    std::string sessionId = generateSessionId();
    _sessions[sessionId] = std::map<std::string, std::string>();
    _sessionExpires[sessionId] = time(NULL) + _sessionTimeout;
    return sessionId;
}

bool SessionManager::isValidSession(const std::string& sessionId) {
    std::map<std::string, time_t>::iterator it = _sessionExpires.find(sessionId);
    if (it == _sessionExpires.end()) {
        return false;
    }
    
    if (time(NULL) > it->second) {
        destroySession(sessionId);
        return false;
    }
    
    // Update expiration time
    it->second = time(NULL) + _sessionTimeout;
    return true;
}

void SessionManager::destroySession(const std::string& sessionId) {
    _sessions.erase(sessionId);
    _sessionExpires.erase(sessionId);
}

void SessionManager::cleanupExpiredSessions() {
    time_t now = time(NULL);
    std::map<std::string, time_t>::iterator it = _sessionExpires.begin();
    
    while (it != _sessionExpires.end()) {
        if (now > it->second) {
            _sessions.erase(it->first);
            _sessionExpires.erase(it++);
        } else {
            ++it;
        }
    }
}

void SessionManager::setSessionData(const std::string& sessionId, const std::string& key, const std::string& value) {
    if (isValidSession(sessionId)) {
        _sessions[sessionId][key] = value;
    }
}

std::string SessionManager::getSessionData(const std::string& sessionId, const std::string& key) {
    if (isValidSession(sessionId)) {
        std::map<std::string, std::string>::iterator it = _sessions[sessionId].find(key);
        if (it != _sessions[sessionId].end()) {
            return it->second;
        }
    }
    return "";
}

bool SessionManager::hasSessionData(const std::string& sessionId, const std::string& key) {
    if (isValidSession(sessionId)) {
        return _sessions[sessionId].find(key) != _sessions[sessionId].end();
    }
    return false;
}

void SessionManager::removeSessionData(const std::string& sessionId, const std::string& key) {
    if (isValidSession(sessionId)) {
        _sessions[sessionId].erase(key);
    }
}

size_t SessionManager::getActiveSessionCount() {
    cleanupExpiredSessions();
    return _sessions.size();
}
