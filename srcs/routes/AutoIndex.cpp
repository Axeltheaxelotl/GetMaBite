#include "AutoIndex.hpp"
#include <dirent.h>
#include <sstream>

std::string AutoIndex::generateAutoIndexPage(const std::string &directoryPath)
{
    DIR *dir = opendir(directoryPath.c_str());
    if (!dir)
    {
        return "<html><body><h1>403 Forbidden</h1></body></html>";
    }

    std::ostringstream html;
    html << "<html><body><h1>Index of " << directoryPath << "</h1><ul>";

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL)
    {
        std::string name = entry->d_name;
        if (name != "." && name != "..")
        {
            html << "<li><a href=\"" << name << "\">" << name << "</a></li>";
        }
    }

    closedir(dir);
    html << "</ul></body></html>";
    return html.str();
}