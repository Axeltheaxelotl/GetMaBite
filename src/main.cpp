#include "core/EpollClasse.hpp"
#include "config/Parser.hpp"
#include "serverConfig/ServerConfig.hpp"
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
    for (std::vector<Server>::const_iterator it = servers.begin(); it != servers.end(); ++it)
    {
        std::string host = "0.0.0.0"; // Valeur par défaut
        if (!it->server_names.empty())
        {
            host = it->server_names[0]; // Utilise le premier nom de serveur comme host
        }
        
        for (std::vector<int>::const_iterator portIt = it->listen_ports.begin(); 
             portIt != it->listen_ports.end(); ++portIt)
        {
            ServerConfig config(host, *portIt);
            serverConfigs.push_back(config);
        }
    }

    // Initialiser et exécuter EpollClasse
    EpollClasse epollServer;
    epollServer.setupServers(serverConfigs, servers);
    epollServer.serverRun();

    return 0;
}