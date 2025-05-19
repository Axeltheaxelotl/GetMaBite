#ifndef TIMEOUTMANAGER_HPP
#define TIMEOUTMANAGER_HPP

#include <map>
#include <vector>
#include <string>

// Fonction utilitaire pour lire le temps d'uptime système (en secondes)
double getUptimeSeconds();

class TimeoutManager {
private:
    int _timeoutSeconds;
    std::map<int, double> _clientTimeouts; // fd -> dernier timestamp d'activité (uptime)
public:
    TimeoutManager(int timeoutSeconds);
    void addClient(int clientFd);
    void removeClient(int clientFd);
    bool isClientTimedOut(int clientFd);
    void updateClientActivity(int clientFd);
    std::vector<int> getTimedOutClients();
};

#endif
