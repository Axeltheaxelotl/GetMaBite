#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>
#include <netinet/in.h>

class Server {
public:
    Server(int port); // Initialise le serveur avec un port
    void start(); // Démarre le serveur
private:
    int port; // Port serveur va écouter
    int server_fd; // Descripteur de fichier du socket du serveur
    struct sockaddr_in address; // Adresse serveur

    void initServer(); // Initialise le serveur (création du socket, bind, listen)
    void handleRequest(int client_fd); // Requête reçue d'un client
    void sendResponse(int client_fd, const std::string &response); // Envoie réponse au client
};

#endif
