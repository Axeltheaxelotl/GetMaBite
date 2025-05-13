#include "TimeoutManager.hpp"
#include <sys/time.h>
#include <cstddef> // Include for NULL
#include <iostream> // For logging

TimeoutManager::TimeoutManager(int timeoutSeconds) : _timeoutSeconds(timeoutSeconds) {}

void TimeoutManager::addClient(int clientFd) {
    struct timeval now;
    gettimeofday(&now, NULL);
    _clientTimeouts[clientFd] = now.tv_sec;
}

void TimeoutManager::removeClient(int clientFd) {
    _clientTimeouts.erase(clientFd);
}

bool TimeoutManager::isClientTimedOut(int clientFd) {
    struct timeval now;
    gettimeofday(&now, NULL);
    if (_clientTimeouts.find(clientFd) != _clientTimeouts.end()) {
        long elapsed = now.tv_sec - _clientTimeouts[clientFd];
        if (elapsed >= _timeoutSeconds) {
            std::cout << "Client " << clientFd << " has been inactive for " << elapsed << " seconds. Timing out." << std::endl;
            return true;
        }
    }
    return false;
}

void TimeoutManager::updateClientActivity(int clientFd) {
    struct timeval now;
    gettimeofday(&now, NULL);
    _clientTimeouts[clientFd] = now.tv_sec;
}
