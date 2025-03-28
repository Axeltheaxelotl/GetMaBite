#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <cstdlib>
#include <cerrno>
#include<algorithm>
#include<cctype>
#include<fstream>
#include<set>
#include<iostream>
#include<sstream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <pthread.h>
#include "../srcs/parser/Parser.hpp"
#include "../srcs/parser/Server.hpp"

std::string urlDecode(const std::string &src) {
    std::string ret;
    for (size_t i = 0; i < src.size(); i++) {
        if (src[i] == '%' && i + 2 < src.size()) {
            std::string hex = src.substr(i + 1, 2);
            // Replace nullptr with NULL for C++98 compatibility.
            char ch = static_cast<char>(std::strtol(hex.c_str(), NULL, 16));
            ret.push_back(ch);
            i += 2;
        } else if (src[i] == '+') {
            ret.push_back(' ');
        } else {
            ret.push_back(src[i]);
        }
    }
    return ret;
}

static int setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

struct ConnData {
    int fd;
    std::string request;
};

static void* handleConnection(void* arg) {
    ConnData* data = static_cast<ConnData*>(arg);
    int connFd = data->fd;
    std::string reqStr = data->request;
    delete data;

    std::istringstream debugStream(reqStr);
    std::string debugLine;
    while (std::getline(debugStream, debugLine)) {
        if (debugLine.rfind("[DEBUG]", 0) == 0) {
            std::cout << debugLine << std::endl;
        }
    }

    std::istringstream reqStream(reqStr);
    std::string reqLine;
    std::getline(reqStream, reqLine);
    // Handle the home page with form and optional CGI output
    // Accept both GET and POST, but process the command (cmd parameter) only from POST body.
    std::string cmd;
    if (reqLine.find("POST /") == 0) {
        // Find the blank line separating header and body.
        size_t headerEnd = reqStr.find("\r\n\r\n");
        if (headerEnd != std::string::npos) {
            cmd = reqStr.substr(headerEnd + 4);
            cmd = urlDecode(cmd);
            // Expecting the body to be "cmd=script.py args...", so remove "cmd=".
            const std::string prefix("cmd=");
            if (cmd.compare(0, prefix.size(), prefix) == 0)
                cmd = cmd.substr(prefix.size());
        }
    } else if (reqLine.find("GET /") == 0) {
        // If GET is used, we don't process any command to keep the URL unchanged.
        cmd = "";
    }
    
    // These values are taken from the server configuration (config.conf)
    // Since the Server class does not currently hold these values,
    // we assign them directly based on the config.
    std::string cgiExtension = ".py";
    std::string cgiExecutable = "/usr/bin/python3";
    
    std::string cgiOutput;
    if (!cmd.empty()) {
        // Split the cmd into script and its arguments.
        std::istringstream iss(cmd);
        std::string script;
        std::string args;
        iss >> script;
        std::getline(iss, args);
        
        // Only process if the script has the CGI extension.
        if (script.size() >= cgiExtension.size() &&
            script.compare(script.size() - cgiExtension.size(), cgiExtension.size(), cgiExtension) == 0) {
            std::string fullPath = "./cgi-stuff-folder/" + script;
            std::string runCmd = cgiExecutable + " " + fullPath + args;
            FILE* fp = popen(runCmd.c_str(), "r");
            if (fp) {
                char buf[128];
                while (fgets(buf, sizeof(buf), fp) != NULL) {
                    std::string line(buf);
                    if (line.rfind("[DEBUG]", 0) == 0) {
                        std::cout << line << std::endl;
                    } else {
                        cgiOutput += line;
                    }
                }
                pclose(fp);
            } else {
                cgiOutput = "Error executing CGI script";
            }
        } else {
            cgiOutput = "Invalid CGI script file extension.";
        }
    }
    
    std::ostringstream html;
    html << "<html><body>"
         << "<form action=\"/\" method=\"POST\">" // Changed to POST so URL remains unchanged.
         << "<input type=\"text\" name=\"cmd\" placeholder=\"Enter CGI script and args (e.g., script.py arg1)\">";
    if (!cmd.empty())
        html << " value=\"" << cmd << "\"";
    html << "<input type=\"submit\" value=\"Run CGI Script\">"
         << "</form>";
    if (!cgiOutput.empty())
        html << "<div>Output: " << cgiOutput << "</div>";
    html << "</body></html>";
    
    std::string page = html.str();
    std::ostringstream respStream;
    respStream << "HTTP/1.1 200 OK\r\n"
               << "Content-Type: text/html\r\n"
               << "Content-Length: " << page.size() << "\r\n\r\n"
               << page;
    std::string response = respStream.str();
    send(connFd, response.c_str(), response.size(), 0);
    close(connFd);
    return NULL;
}

int main() {
	std::vector<Server> servers = parseConfig("/home/smasse/Projects/P6/GetMaBite/config.conf");
    if (servers.empty()) {
        std::cerr << "No servers configured.\n";
        return 1;
    }
    Server &srv = servers[0];
    if (srv.listen_ports.empty()) {
        std::cerr << "No port to listen on.\n";
        return 1;
    }
    int listenPort = srv.listen_ports[0];
    int listenFd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenFd < 0) {
        std::cerr << "socket() failed: " << strerror(errno) << "\n";
        return 1;
    }
    setNonBlocking(listenFd);

    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(listenPort);

	if (bind(listenFd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
		std::cerr << "bind() failed on port " << listenPort << ": " << strerror(errno) << "\n";
		listenPort += 1;
		addr.sin_port = htons(listenPort);
		if (bind(listenFd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
			std::cerr << "bind() failed on port " << listenPort << ": " << strerror(errno) << "\n";
			close(listenFd);
			return 1;
		}
	}
	if (listen(listenFd, 10) < 0) {
        std::cerr << "listen() failed: " << strerror(errno) << "\n";
        close(listenFd);
        return 1;
    }

    int epollFd = epoll_create(1);
    if (epollFd < 0) {
        std::cerr << "epoll_create() failed: " << strerror(errno) << "\n";
        close(listenFd);
        return 1;
    }

    epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = listenFd;
    epoll_ctl(epollFd, EPOLL_CTL_ADD, listenFd, &ev);

    std::cout << "Server listening on port " << listenPort << "...\n";

    while (true) {
        epoll_event events[10];
        int nfds = epoll_wait(epollFd, events, 10, -1);
        if (nfds < 0) {
            std::cerr << "epoll_wait() failed: " << strerror(errno) << "\n";
            break;
        }
        for (int i = 0; i < nfds; ++i) {
            if (events[i].data.fd == listenFd) {
                while (true) {
                    sockaddr_in clientAddr;
                    socklen_t clientSize = sizeof(clientAddr);
                    int connFd = accept(listenFd, (struct sockaddr*)&clientAddr, &clientSize);
                    if (connFd < 0) {
                        if (errno != EAGAIN && errno != EWOULDBLOCK)
                            std::cerr << "accept() failed: " << strerror(errno) << "\n";
                        break;
                    }
                    setNonBlocking(connFd);
                    epoll_event connEv;
                    connEv.events = EPOLLIN;
                    connEv.data.fd = connFd;
                    epoll_ctl(epollFd, EPOLL_CTL_ADD, connFd, &connEv);
                }
            } else if (events[i].events & EPOLLIN) {
                int connFd = events[i].data.fd;
                char buffer[1024];
                int bytesRead = read(connFd, buffer, sizeof(buffer) - 1);
                if (bytesRead > 0) {
                    buffer[bytesRead] = '\0';
                    std::string reqStr(buffer);

                    epoll_ctl(epollFd, EPOLL_CTL_DEL, connFd, NULL);

                    ConnData* data = new ConnData;
                    data->fd = connFd;
                    data->request = reqStr;

                    pthread_t tid;
                    pthread_create(&tid, NULL, handleConnection, data);
                    pthread_detach(tid);
                }
            }
        }
    }
    close(epollFd);
    close(listenFd);
    return 0;
}
