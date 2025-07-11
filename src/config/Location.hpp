#ifndef LOCATION_HPP
#define LOCATION_HPP

#include <string>
#include <vector>
#include <map>

class Location {
public:
    std::string path;                    // Le chemin de la location (ex: "/upload")
    std::string root;                    // Répertoire racine pour cette location
    std::string alias;                   // Alias pour cette location
    std::string index;                   // Fichier index par défaut
    std::vector<std::string> allow_methods; // Méthodes HTTP autorisées
    bool autoindex;                      // Autoindex on/off
    std::string upload_path;             // Chemin pour les uploads
    std::string return_url;              // URL de redirection
    int return_code;                     // Code de redirection (301, 302, etc.)
    std::map<std::string, std::string> cgi_extensions; // Extensions CGI et leurs interpréteurs
    size_t client_max_body_size;         // Taille max du corps de requête

    Location();
    ~Location();
    
    // Méthodes utilitaires
    bool isMethodAllowed(const std::string& method) const;
    std::string getCgiInterpreter(const std::string& extension) const;
};

#endif