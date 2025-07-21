#ifndef SERVER_HPP
#define SERVER_HPP

#include<string>
#include<vector>
#include<map>
#include"Location.hpp"

class Server {
public:
    std::vector<int> listen_ports;              // Ports d'écoute
    std::vector<std::string> server_names;      // Noms de serveur
    std::string root;                           // Répertoire racine
    std::string index;                          // Fichier index par défaut
    std::map<int, std::string> error_pages;     // Pages d'erreur personnalisées
    size_t client_max_body_size;                // Taille max du corps de requête
    std::vector<Location> locations;            // Locations configurées
    std::map<std::string, std::string> cgi_extensions; // Extensions CGI globales
    bool autoindex;                             // Autoindex global
    std::vector<std::string> allow_methods; // Méthodes HTTP autorisées
    std::string upload_path;                    // Chemin d'upload par défaut
    
    Server();
    ~Server();
    
    // Méthodes utilitaires
    Location* findLocation(const std::string& path);
    const Location* findLocation(const std::string& path) const;
    std::string getErrorPage(int error_code) const;
    bool isMethodAllowedForPath(const std::string& path, const std::string& method) const;
    std::string getCgiInterpreterForPath(const std::string& path, const std::string& extension) const;
};

#endif
