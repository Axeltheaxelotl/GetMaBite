#ifndef TIMEOUTMANAGER_HPP
#define TIMEOUTMANAGER_HPP

#include <map>
#include <vector> // Added for returning timed-out clients
#include <ctime>  // Added for time_t

class TimeoutManager {
private:
    int _timeoutSeconds;
    std::map<int, time_t> _clientTimeouts; // Changed to use time_t for timestamps

public:
    TimeoutManager(int timeoutSeconds);
    void addClient(int clientFd);
    void removeClient(int clientFd);
    bool isClientTimedOut(int clientFd);
    void updateClientActivity(int clientFd);
    std::vector<int> getTimedOutClients(); // New method to retrieve timed-out clients
};

#endif // TIMEOUTMANAGER_HPP
