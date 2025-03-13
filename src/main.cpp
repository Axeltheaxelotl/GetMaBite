#include "server.hpp"

int main()
{
    int port = 8080; // Choisir un port pour le serveur
    Server server(port); // Créer une instance de la classe Server
    server.start(); // Démarrer le serveur
    return 0;
}
