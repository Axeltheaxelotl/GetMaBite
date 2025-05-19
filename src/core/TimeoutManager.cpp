#include "TimeoutManager.hpp"
#include <fstream>
#include <sstream>
#include <vector>
#include <cstddef>
#include <iostream>

// Lit le temps d'uptime système (en secondes)
double getUptimeSeconds()
{
    std::ifstream uptimeFile("/proc/uptime");
    std::string line;
    if (std::getline(uptimeFile, line))
    {
        std::istringstream iss(line);
        double uptime = 0.0;
        iss >> uptime;
        return uptime;
    }
    return 0.0;
}

// Constructeur
TimeoutManager::TimeoutManager(int timeoutSeconds)
    : _timeoutSeconds(timeoutSeconds) {}

// Ajoute un client dans la map
void TimeoutManager::addClient(int clientFd)
{
    _clientTimeouts[clientFd] = getUptimeSeconds();
}

// Retire un client de la map
void TimeoutManager::removeClient(int clientFd)
{
    _clientTimeouts.erase(clientFd);
}

// Vérifie si le client a dépassé le timeout
bool TimeoutManager::isClientTimedOut(int clientFd)
{
    double now = getUptimeSeconds();
    if (_clientTimeouts.find(clientFd) != _clientTimeouts.end())
    {
        return (now - _clientTimeouts[clientFd]) >= _timeoutSeconds;
    }
    return false;
}

// Met à jour l'activité d'un client
void TimeoutManager::updateClientActivity(int clientFd)
{
    _clientTimeouts[clientFd] = getUptimeSeconds();
}

// Retourne la liste des clients qui ont dépassé le timeout
std::vector<int> TimeoutManager::getTimedOutClients()
{
    std::vector<int> timedOutClients;
    double now = getUptimeSeconds();
    for (std::map<int, double>::iterator it = _clientTimeouts.begin(); it != _clientTimeouts.end(); ++it)
    {
        if ((now - it->second) >= _timeoutSeconds)
        {
            timedOutClients.push_back(it->first);
        }
    }
    return timedOutClients;
}
