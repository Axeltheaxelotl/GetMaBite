#include "server.hpp"

int main()
{
    int port = 8080;
    Server server(port);
    server.start();
    return 0;
}
