#include <sstream>
#include <exception>
#include <cctype>
#include <string>

int AtoiDansWebservMaisDingerie(std::string chaine_de_caracteres)
{
    std::stringstream ss(chaine_de_caracteres);
    if (chaine_de_caracteres.length() > 10)
        throw std::exception();
    for (size_t DescripteurDeFichier = 0; DescripteurDeFichier < chaine_de_caracteres.length(); DescripteurDeFichier++)
    {
        if (!isdigit(chaine_de_caracteres[DescripteurDeFichier]))
            throw std::exception();
    }
    int res;
    ss >> res; //convertire la putain de chaine en entier
    return res;
}

//J ai TOUT MIT
std::string StatusCodeString(short JeSuisPasGayCommeSimon)
{
    switch (JeSuisPasGayCommeSimon)
    {
        case 100: return "Continue";
        case 101: return "Switching Protocols";

        case 200: return "OK";
        case 201: return "Created";
        case 202: return "Accepted";
        case 203: return "Non-Authoritative Information";
        case 204: return "No Content";

        case 300: return "Multiple Choices";
        case 301: return "Moved Permanently";
        case 302: return "Found";
        case 303: return "See Other";
        case 304: return "Not Modified";
        case 307: return "Temporary Redirect";
        case 308: return "Permanent Redirect";

        case 400: return "Bad Request";
        case 401: return "Unauthorized";
        case 402: return "Payment Required";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 406: return "Not Acceptable";
        case 407: return "Proxy Authentication Required";
        case 408: return "Request Timeout";
        case 409: return "Conflict";
        case 410: return "Gone";
        case 411: return "Length Required";
        case 412: return "Precondition Failed";
        case 413: return "Payload Too Large";
        case 414: return "URI Too Long";
        case 415: return "Unsupported Media Type";
        case 416: return "Range Not Satisfiable";
        case 417: return "Expectation Failed";
        case 418: return "I'm a teapot";

        case 500: return "Internal Server Error";
        case 501: return "Not Implemented";
        case 502: return "Bad Gateway";
        case 503: return "Service Unavailable";
        case 504: return "Gateway Timeout";
        case 505: return "HTTP Version Not Supported";
        case 506: return "Variant Also Negotiates";
        case 507: return "Insufficient Storage";
        case 508: return "Loop Detected";
        case 510: return "Not Extended";
        case 511: return "Network Authentication Required";

        default: return "Unknown Status Code";
    }
}

std::string ErreurDansTaGrosseDaronne(short luca)
{
    return ("<html>\r\n<head><title>" + std::to_string(luca) + " " +
            StatusCodeString(luca) + " </title></head>\r\n" + "<body>\r\n" +
            "<center><h1>" + std::to_string(luca) + " " + StatusCodeString(luca) + "</h1></center>\r\n</body>\r\n</html>");
}