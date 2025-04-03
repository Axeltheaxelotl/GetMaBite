#include "epollDansTaGrosseDaronne/EpollClasse.hpp"
#include "parser/Parser.hpp"
#include "serverConfig/ServerConfig.hpp"
#include "routes/RedirectionHandler.hpp" // Ajout de l'inclusion pour RedirectionHandler
#include <vector>
#include <iostream>

int main(int argc, char **argv)
{
    // Charger le fichier de configuration
    std::string configPath = "config.conf";
    if (argc > 1)
        configPath = argv[1];

    // Parser la configuration
    std::vector<Server> servers = parseConfig(configPath);
    if (servers.empty())
    {
        std::cerr << "Erreur : Impossible de charger la configuration." << std::endl;
        return 1;
    }

    // Convertir les objets Server en ServerConfig
    std::vector<ServerConfig> serverConfigs;
    for (size_t i = 0; i < servers.size(); ++i)
    {
        for (size_t j = 0; j < servers[i].listen_ports.size(); ++j)
        {
            serverConfigs.push_back(ServerConfig("0.0.0.0", servers[i].listen_ports[j]));
        }
    }

    // Exemple de test pour les redirections HTTP
    std::string redirect301 = RedirectionHandler::generateRedirectReponse(301, "https://example.com");
    std::string redirect302 = RedirectionHandler::generateRedirectReponse(302, "https://example.org");

    std::cout << "301 Redirect:\n" << redirect301 << "\n";
    std::cout << "302 Redirect:\n" << redirect302 << "\n";

    // Initialiser et exécuter EpollClasse
    EpollClasse epollServer;
    epollServer.setupServers(serverConfigs);
    epollServer.serverRun();

    return 0;
}