#include"Location.hpp"

Location::Location() : autoindex(false), return_code(0), client_max_body_size(1048576) {} // 1MB par défaut

Location::~Location() {}

bool Location::isMethodAllowed(const std::string& method) const {
    if (allow_methods.empty()) {
        // Si aucune méthode n'est spécifiée, autoriser GET, POST, DELETE par défaut
        return (method == "GET" || method == "POST" || method == "DELETE");
    }
    
    for (std::vector<std::string>::const_iterator it = allow_methods.begin(); 
         it != allow_methods.end(); ++it) {
        if (*it == method) {
            return true;
        }
    }
    return false;
}

std::string Location::getCgiInterpreter(const std::string& extension) const {
    std::map<std::string, std::string>::const_iterator it = cgi_extensions.find(extension);
    if (it != cgi_extensions.end()) {
        return it->second;
    }
    return "";
}
