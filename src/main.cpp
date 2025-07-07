#include "core/EpollClasse.hpp"
#include "config/Parser.hpp"
#include "serverConfig/ServerConfig.hpp"
#include <vector>
#include <iostream>

int main(int argc, char **argv)
{
    try {
        // Charger le fichier de configuration
        std::string configPath = "config.conf";
        if (argc > 1)
            configPath = argv[1];

        // Parser la configuration
        std::vector<Server> servers = parseConfig(configPath);
        if (servers.empty())
            throw std::runtime_error("Impossible de charger la configuration.");

        // Convertir les objets Server en ServerConfig
        std::vector<ServerConfig> serverConfigs;
        for (std::vector<Server>::const_iterator it = servers.begin(); it != servers.end(); ++it)
        {
            std::string host = "0.0.0.0"; // Valeur par défaut
            if (!it->server_names.empty())
                host = it->server_names[0];

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
    }
    catch (const std::exception& e) {
        std::cerr << "Erreur : " << e.what() << std::endl;
        return 1;
    }

    return 0;
}