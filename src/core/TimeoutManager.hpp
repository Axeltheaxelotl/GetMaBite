#ifndef TIMEOUTMANAGER_HPP
#define TIMEOUTMANAGER_HPP

#include <map>

class TimeoutManager {
private:
    int _timeoutSeconds;
    std::map<int, long> _clientTimeouts; // Map client FD to last activity timestamp

public:
    TimeoutManager(int timeoutSeconds);
    void addClient(int clientFd);
    void removeClient(int clientFd);
    bool isClientTimedOut(int clientFd);
    void updateClientActivity(int clientFd);
};

#endif // TIMEOUTMANAGER_HPP
