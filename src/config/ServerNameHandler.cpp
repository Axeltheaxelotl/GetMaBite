#include "ServerNameHandler.hpp"
#include <algorithm>

bool ServerNameHandler::isServerNameMatch(const std::vector<std::string>& serverNames, const std::string& host) {
    return std::find(serverNames.begin(), serverNames.end(), host) != serverNames.end();
}
/*
 * Le support des server_names multiple permet a un serveur HTTP d'associer plusieurs noms d'hote
 * (tamere.com, www.tamere.com, etc.) a une meme configuration de serveur. Cela est utile pour gerer plusieurs
 * domaines ou sous-domaines avec une seule instance de serveur.
*/

