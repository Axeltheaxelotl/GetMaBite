#include "server.hpp"
#include <iostream>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fstream>

Server::Server(int port) : port(port)
{
    initServer();
}

void Server::initServer()
{
    // Création du socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1)
    {
        std::cerr << "Erreur lors de la création du socket" << std::endl;
        exit(EXIT_FAILURE);
    }

    // Préparer l'adresse
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY; // Accepte les connexions depuis toutes les adresses IP
    address.sin_port = htons(port); // Convertit le port en format réseau

    // Lier le socket à l'adresse
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        std::cerr << "Erreur lors du bind" << std::endl;
        exit(EXIT_FAILURE);
    }

    // Mettre le serveur en écoute
    if (listen(server_fd, 3) < 0)
    {
        std::cerr << "Erreur lors de l'écoute" << std::endl;
        exit(EXIT_FAILURE);
    }
}

void Server::handleRequest(int client_fd)
{
    char buffer[1024] = {0};
    int valread = read(client_fd, buffer, 1024); // Lire la requête du client
    if (valread == -1)
    {
        std::cerr << "Erreur de lecture" << std::endl;
        return;
    }

    std::cout << "Requête reçue:\n" << buffer << std::endl;

    // Vérifier la méthode HTTP
    std::string request(buffer);
    if (request.find("GET") != std::string::npos)
    {
        handleGET(client_fd);
    }
    else if (request.find("POST") != std::string::npos)
    {
        handlePOST(client_fd);
    }
    else if (request.find("DELETE") != std::string::npos)
    {
        handleDELETE(client_fd);
    }
    else
    {
        std::string response = "HTTP/1.1 405 Method Not Allowed\r\n\r\n";
        sendResponse(client_fd, response);
    }
}

void Server::handleGET(int client_fd)
{
    std::string filepath = "./index.html";  // Exemple de fichier à renvoyer
    std::ifstream file(filepath);

    if (file)
    {
        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        std::string response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n" + content;
        sendResponse(client_fd, response);
    }
    else
    {
        std::string response = "HTTP/1.1 404 Not Found\r\n\r\nFile not found.";
        sendResponse(client_fd, response);
    }
}

void Server::handlePOST(int client_fd)
{
    // Gestion des requêtes POST (ex. téléchargement de fichier)
    std::string response = "HTTP/1.1 200 OK\r\n\r\nPOST request received.";
    sendResponse(client_fd, response);
}

void Server::handleDELETE(int client_fd)
{
    // Suppression d'un fichier (ex. si le client demande la suppression d'un fichier)
    std::string response = "HTTP/1.1 200 OK\r\n\r\nDELETE request received.";
    sendResponse(client_fd, response);
}

void Server::sendResponse(int client_fd, const std::string &response)
{
    send(client_fd, response.c_str(), response.length(), 0); // Envoyer la réponse HTTP
    close(client_fd); // Fermer la connexion avec le client
}

void Server::start()
{
    std::cout << "Serveur démarré sur le port " << port << "...\n";
    int addrlen = sizeof(address);
    while (true)
    {
        int client_fd = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
        if (client_fd < 0)
        {
            std::cerr << "Erreur lors de l'acceptation de la connexion" << std::endl;
            continue;
        }
        handleRequest(client_fd); // Gérer la requête du client
    }
}
