#ifndef MULTIPARTPARSER_HPP
#define MULTIPARTPARSER_HPP

#include <string>
#include <vector>

// Structure représentant une partie dans un upload multipart
struct MultipartPart {
    std::string name;
    std::string filename;    // vide si champ simple
    std::string contentType; // vide si non spécifié
    std::string data;
};

class MultipartParser {
public:
    // Analyse le corps `body` selon la `boundary` (sans les '--')
    static std::vector<MultipartPart> parse(const std::string& body,
                                            const std::string& boundary);
};

#endif // MULTIPARTPARSER_HPP
