#include "TimeoutManager.hpp"
#include <ctime> // Added for time_t
#include <vector>
#include <sys/time.h>
#include <cstddef> // Include for NULL
#include <iostream> // For logging

TimeoutManager::TimeoutManager(int timeoutSeconds) : _timeoutSeconds(timeoutSeconds) {}

void TimeoutManager::addClient(int clientFd) {
    _clientTimeouts[clientFd] = time(NULL); // Use time(NULL) for current timestamp
}

void TimeoutManager::removeClient(int clientFd) {
    _clientTimeouts.erase(clientFd);
}

bool TimeoutManager::isClientTimedOut(int clientFd) {
    time_t now = time(NULL);
    if (_clientTimeouts.find(clientFd) != _clientTimeouts.end()) {
        return (now - _clientTimeouts[clientFd]) >= _timeoutSeconds;
    }
    return false;
}

void TimeoutManager::updateClientActivity(int clientFd) {
    _clientTimeouts[clientFd] = time(NULL); // Update timestamp to current time
}

std::vector<int> TimeoutManager::getTimedOutClients() {
    std::vector<int> timedOutClients;
    time_t now = time(NULL);
    for (std::map<int, time_t>::iterator it = _clientTimeouts.begin(); it != _clientTimeouts.end(); ++it) {
        if ((now - it->second) >= _timeoutSeconds) {
            timedOutClients.push_back(it->first);
        }
    }
    return timedOutClients;
}
