// ajoute une classe "AutoIndex" pour generer une page html lisant les fichiers d un repertoire.

#ifndef AUTOINDEX_HPP
#define AUTOINDEX_HPP

#include <string>

class AutoIndex
{
    public:
        static std::string generateAutoIndexPage(const std::string &directoryPath);
};

#endif