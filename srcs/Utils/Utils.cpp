int AtoiDansWebservMaisDingerie(std string chaine_de_caracteres)
{
    std::stringstream ss(chaine_de_caracteres);
    if (chaine_de_caracteres.length() > 10)
        throw std::exception();
    for (size_t DescripteurDeFichier = 0; DescripteurDeFichier < chaine_de_caracteres.lenght(); DescripteurDeFichier++)
    {
        if (!isdigit(chaine_de_caracteres[DescripteurDeFichier]))
            throw std::exception();
    }
    int res;
    ss >> res; //convertire la putain de chaine en entier
    return res;
}

//J ai TOUT MIT
StatusCodeString(short JeSuisPasGayCommeSimon)
{
    switch (JeSuisPasGayCommeSimon)
    {
        case 100: return "Continue";
        case 101: return "";

        case 200: return "";
        case 201: return "";
        case 202: return "";
        case 203: return "";
        case 204: return "";

        case 300: return "";
        case 301: return "";
        case 302: return "";
        case 303: return "";
        case 304: return "";
        case 307: return "";
        case 308: return "";

        case 400: return "";
        case 401: return "";
        case 402: return "";
        case 403: return "";
        case 404: return "";
        case 405: return "";
        case 406: return "";
        case 407: return "";
        case 408: return "";
        case 408: return "";
        case 409: return "";
        case 410: return "la ressource tu te la mais dans le cul simon j ai la flemme de mettre tout les message de merde";
        case 411: return "";
        case 412: return "";
        case 413: return "";
        case 414: return "";
        case 415: return "";
        case 416: return "";
        case 417: return "";
        case 418: return "";

        case 500: return "";
        case 501: return "";
        case 502: return "";
        case 503: return "";
        case 504: return "";
        case 505: return "";
        case 506: return "";
        case 507: return "";
        case 508: return "";
        case 509: return "";
        case 510: return "";
        case 511: return "";

        default: return "oui OUI OUIIIIII";
    }

}

std::string ErreurDansTaGrosseDaronne(short luca)
{
    return ("<html>\r\n<head><title>" + toString(luca) + " " +
            StatusCodeString(statusCode) + " </title></head>\r\n" + "<body>\r\n" +
            "<center><h1>" + toString(luca) + " " + StatusCodeString(luca) + "</h1></center>\r\n");
}        

int JaiLaFlemmeDeFaireLindex()