#include "MultipartParser.hpp"
#include <sstream>
#include <algorithm>

std::vector<MultipartPart> MultipartParser::parse(const std::string& body,
                                                  const std::string& boundary)
{
    std::vector<MultipartPart> parts;
    std::string delim = "--" + boundary;
    size_t pos = 0;
    // skip preamble
    while ((pos = body.find(delim, pos)) != std::string::npos) {
        pos += delim.size();
        if (body.substr(pos, 2) == "--") break; // end
        // skip CRLF
        if (body.substr(pos, 2) == "\r\n") pos += 2;
        size_t header_end = body.find("\r\n\r\n", pos);
        if (header_end == std::string::npos) break;
        std::string headers = body.substr(pos, header_end - pos);
        pos = header_end + 4;
        size_t part_end = body.find(delim, pos);
        if (part_end == std::string::npos) break;
        std::string data = body.substr(pos, part_end - pos - 2); // remove ending CRLF

        MultipartPart part;
        std::istringstream hs(headers);
        std::string line;
        while (std::getline(hs, line) && !line.empty()) {
            if (line.find("Content-Disposition:") == 0) {
                size_t n1 = line.find("name=\"");
                size_t n2 = line.find('"', n1 + 6);
                part.name = line.substr(n1 + 6, n2 - (n1 + 6));
                size_t f1 = line.find("filename=\"");
                if (f1 != std::string::npos) {
                    size_t f2 = line.find('"', f1 + 10);
                    part.filename = line.substr(f1 + 10, f2 - (f1 + 10));
                }
            } else if (line.find("Content-Type:") == 0) {
                part.contentType = line.substr(14);
            }
        }
        part.data = data;
        parts.push_back(part);
        pos = part_end;
    }
    return parts;
}
