#include "ServerRouter.hpp"
#include <algorithm>

int ServerRouter::findServerIndex(const std::vector<Server>& serverConfigs, const std::string& host, int port)
{
    int defaultIndex = -1;
    for (size_t i = 0; i < serverConfigs.size(); ++i) {
        const Server& srv = serverConfigs[i];
        // Vérifie si ce serveur écoute sur ce port
        if (std::find(srv.listen_ports.begin(), srv.listen_ports.end(), port) != srv.listen_ports.end()) {
            // Si c'est le premier pour ce port, c'est le serveur par défaut
            if (defaultIndex == -1)
                defaultIndex = i;
            // Cherche un match exact sur le host
            for (size_t j = 0; j < srv.server_names.size(); ++j) {
                if (srv.server_names[j] == host)
                    return i;
            }
        }
    }
    // Si aucun match sur le host, retourne le serveur par défaut pour ce port
    return defaultIndex;
}
