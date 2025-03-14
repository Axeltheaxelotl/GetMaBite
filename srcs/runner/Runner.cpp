#include "Runner.hpp"
#include "../parser/Server.hpp"
#include "../parser/Parser.hpp"
#include <iostream>
#include <unistd.h>
#include <string>
#include <fstream>
#include <cstring>
#include <vector>
#include <sstream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <poll.h>
#include <fcntl.h>
#include <cstdlib>

// Fonction to_string personnalisée
static std::string to_string(int value)
{
    std::ostringstream oss;
    oss << value;
    return oss.str();
}

static void setNonBlocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
    {
        std::cerr << "fcntl F_GETFL a échoué" << std::endl;
        exit(EXIT_FAILURE);
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
    {
        std::cerr << "fcntl F_SETFL a échoué" << std::endl;
        exit(EXIT_FAILURE);
    }
}

static void sendResponse(int client_fd, const std::string &response)
{
    send(client_fd, response.c_str(), response.size(), 0);
    close(client_fd);
}

static void sendErrorResponse(int client_fd, int statusCode, const std::string &statusMessage)
{
    std::string errorPagePath = "./www/errors/" + to_string(statusCode) + ".html";
    std::ifstream file(errorPagePath.c_str());
    std::string content;

    if (file)
    {
        content.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    }
    else
    {
        // Si le fichier d'erreur n'est pas trouvé, créer une réponse par défaut.
        content = "<html><body><h1>" + to_string(statusCode) + " " + statusMessage + "</h1></body></html>";
    }

    std::string response = "HTTP/1.1 " + to_string(statusCode) + " " + statusMessage + "\r\nContent-Type: text/html\r\n\r\n" + content;
    sendResponse(client_fd, response);
}


static void handleGET(int client_fd, const std::string &indexPath)
{
    std::ifstream file(indexPath.c_str());
    if (file)
    {
        std::string content((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());
        std::string response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n" + content;
        sendResponse(client_fd, response);
    }
    else
    {
        std::cerr << "Erreur 404: Fichier non trouvé" << std::endl;
        sendErrorResponse(client_fd, 404, "Not Found"); // Removed the fourth argument
    }
}


static void handlePOST(int client_fd)
{
    std::string response = "HTTP/1.1 200 OK\r\n\r\nPOST request received.";
    sendResponse(client_fd, response);
}

static void handleDELETE(int client_fd)
{
    std::string response = "HTTP/1.1 200 OK\r\n\r\nDELETE request received.";
    sendResponse(client_fd, response);
}

static void handleRequest(int client_fd, const std::string &indexPath)
{
    char buffer[1024] = {0};
    int valread = read(client_fd, buffer, sizeof(buffer));
    if (valread <= 0)
    {
        std::cerr << "Lecture échouée ou connexion fermée par le client" << std::endl;
        close(client_fd);
        return;
    }
    std::string request(buffer);
    std::cout << "Requête reçue: " << request << std::endl;

    // Vérifier la méthode HTTP
    if (request.find("GET") != std::string::npos)
    {
        std::cout << "Traitement de la requête GET" << std::endl;
        handleGET(client_fd, indexPath);
    }
    else if (request.find("POST") != std::string::npos)
    {
        std::cout << "Traitement de la requête POST" << std::endl;
        handlePOST(client_fd);
    }
    else if (request.find("DELETE") != std::string::npos)
    {
        std::cout << "Traitement de la requête DELETE" << std::endl;
        handleDELETE(client_fd);
    }
    else
    {
        // Si la méthode n'est pas supportée, renvoyer une erreur 405
        std::cerr << "Erreur 405: Méthode non autorisée" << std::endl;
        sendErrorResponse(client_fd, 405, "Method Not Allowed");
    }
    
    // Vérifier si l'URL est invalide (par exemple un chemin incorrect)
    if (request.find("invalid request") != std::string::npos)
    {
        std::cerr << "Erreur 400: Requête invalide" << std::endl;
        sendErrorResponse(client_fd, 400, "Bad Request");
    }
    // Vous pouvez ajouter d'autres conditions pour les erreurs 401, 403, 404, etc.
}


Runner::Runner()
{
}

Runner::~Runner()
{
}

void Runner::run(int argc, char **argv)
{
    std::string configPath = "config.conf";
    if (argc > 1)
        configPath = argv[1];
    std::vector<Server> servers = parseConfig(configPath);
    if (servers.empty())
    {
        std::cerr << "Échec du chargement de la configuration." << std::endl;
        return;
    }
    Server config = servers[0];
    if (config.listen_ports.empty())
    {
        std::cerr << "Aucun port spécifié dans la configuration." << std::endl;
        return;
    }
    int port = config.listen_ports[0];
    std::string indexPath = config.index.empty() ? "./www/index.html" : config.index;
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0)
    {
        std::cerr << "Erreur de création de socket" << std::endl;
        return;
    }
    setNonBlocking(server_fd);
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        std::cerr << "Échec du bind" << std::endl;
        return;
    }
    if (listen(server_fd, 3) < 0)
    {
        std::cerr << "Échec du listen" << std::endl;
        return;
    }
    std::cout << "Configuration du serveur:" << std::endl;
    std::cout << "Port: " << port << std::endl;
    std::cout << "Chemin de l'index: " << indexPath << std::endl;
    std::cout << "Serveur en cours d'exécution sur le port " << port << "..." << std::endl;

    struct pollfd fds[200];
    int nfds = 1;
    fds[0].fd = server_fd;
    fds[0].events = POLLIN;

    while (true)
    {
        int poll_count = poll(fds, nfds, -1);
        if (poll_count < 0)
        {
            std::cerr << "Échec du poll" << std::endl;
            return;
        }
        for (int i = 0; i < nfds; i++)
        {
            if (fds[i].revents & POLLIN)
            {
                if (fds[i].fd == server_fd)
                {
                    struct sockaddr_in client_address;
                    socklen_t addrlen = sizeof(client_address);
                    int client_fd = accept(server_fd, (struct sockaddr *)&client_address, &addrlen);
                    if (client_fd < 0)
                    {
                        std::cerr << "Échec de l'accept" << std::endl;
                        continue;
                    }
                    setNonBlocking(client_fd);
                    fds[nfds].fd = client_fd;
                    fds[nfds].events = POLLIN;
                    nfds++;
                    std::cout << "Connexion de " << inet_ntoa(client_address.sin_addr)
                              << ":" << ntohs(client_address.sin_port) << std::endl;
                }
                else
                {
                    handleRequest(fds[i].fd, indexPath);
                    close(fds[i].fd);
                    fds[i] = fds[nfds - 1];
                    nfds--;
                }
            }
        }
    }
}