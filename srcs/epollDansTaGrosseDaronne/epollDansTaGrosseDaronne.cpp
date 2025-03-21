#include "epollDansTagrosseDaronne.hpp"

// constructeur du epollDansTaGrosseDaronne, epoll_fd

epollDansTaGrosseDaronne::epollDansTaGrosseDaronne()
{
    _epoll_fd = epoll_create1(0); Cree une instance d epoll
    if (_epoll_fd == -1)
    {
        Logger::logMsg(RED, CONSOLE_OUTPUT, "Epoll dans le cul: %s", strerror(errno));
        exit(1);
    }
    _biggest_fd = 0; // plus grand descripeur de fichier
}

epollDansTaGrosseDaronne::~epollDansTaGrosseDarone()
{
    close(_epoll_fd); //ferme epoll_fd quand le serveur est art
}

// initialise les serveurs a partir de la conf
// std::vector<ServerConfig> servers, le parametre est une liste de serveurs
// en vecteur d objet ServerConfig, chaque vecteur contient des informations comme le
// nom du serveur, IP, port, etc ... 
void epollDansTaGrosseDaronne::setupServers(std::vector<ServerConfig> servers)
{
    // Logger::LogMsg enregistre un message dans console
    // it.getServerName().c_str() la methode getServerName revoie le nom du serveur sous forme
    // de chaine de caracteres, c_str convertit std::string une chaine C-style pour l aff dans le log
    Logger::LogMsg(LIGHTMAGENTA, CONSOLE_OUTPUT, "OUIIIIiiiiii....");
    _servers = servers;

    // pour chaque serveurs dans list configurer
    for (auto &it : _servers)
    {
        it.setupServer(); // chaque serveur
        Logger::logMsg(LIGHTMAGENTA, CONSOLE_OUTPUT, "Server Created: %s", it.getServerName().c_str());    }
}