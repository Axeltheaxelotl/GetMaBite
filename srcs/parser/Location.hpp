#ifndef LOCATION_HPP
#define LOCATION_HPP

#include <string>
#include <vector>
#include <map>

class Location {
public:
	std::string                      path;
	std::string                      root;
	std::string                      alias;
	std::string                      index;
	std::vector<std::string>         allow_methods;
	std::map<std::string, std::string> cgi_extensions;
	std::string                      upload_path;
	bool                             autoindex;
	int                              return_code;
	std::string                      return_url;

	Location();
	~Location();
};

#endif
