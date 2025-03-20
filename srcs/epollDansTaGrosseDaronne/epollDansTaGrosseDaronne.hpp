#ifndef EPOLLDANSTAGROSSEDARONNE_HPP
#define EPOLLDANSTAGROSSEDARONNE_HPP

#include <vector>
#include <map> // std::map
#include <string>
#include <sys/epoll.h> // pour epoll
#include <unistd.h> // systemes de base
#include <fcntl.h> // operatoins controle fichiers
#include <netinet/in.h> //structure sockaddr_in, constantes reseaux
#include <cstring>
#include <errno.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <sys/wait.h>

#include <> //Classe Client pour les clients connectes
#include <> // Classe pour stocker la configuration des serveurs
#include <> // Pour enregistrer des messages dans la console
#include <> // CGI

#define BUFFER 1024 // taille message communications
#define TIMEOUT 60  // fermer connection inactive

// pour gerer les serveurs, les connexions, et les evenement reseau
class epollDansTaGrosseDarone
{
    public:
        
        epollDansTaGrosseDarone();

        ~epollDansTaGrosseDarone();

        //initialise les serveur a partir de la conf
        void ewfrwer();

        //pour executer la boucle principale du serveur en utiliant epoll pour evenement
        void wrfkeorgk();

        //pour verifier les connexions client inactives et fermer
        void eferg();

        //pour initialiser epoll pour gerer les evenements de lecture et d'ecriture
        void efiwerh();

        //accepter une nouvelle connexion client
        void jegrth(&serv); 

        //fermer connxion client
        void rfheroih(const int i);

        //pour envoyer reponse au client
        void fahi(const int i, ...);

        //


}