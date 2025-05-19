#include "RequestBufferManager.hpp"
#include "../config/Server.hpp"
#include <cstdlib>
#include <cstdio>

void RequestBufferManager::append(int client_fd, const std::string& data) {
    buffers[client_fd] += data;
}

std::string& RequestBufferManager::get(int client_fd) {
    return buffers[client_fd];
}

void RequestBufferManager::clear(int client_fd) {
    buffers[client_fd].clear();
}

void RequestBufferManager::remove(int client_fd) {
    buffers.erase(client_fd);
}

bool RequestBufferManager::isRequestComplete(int client_fd, const Server& server) {
    std::string& buf = buffers[client_fd];
    size_t header_end = buf.find("\r\n\r\n");
    if (header_end == std::string::npos)
        return false; // headers incomplets

    // Vérifie Content-Length si POST/PUT
    size_t content_length_pos = buf.find("Content-Length:");
    if (content_length_pos != std::string::npos) {
        size_t value_start = buf.find_first_of("0123456789", content_length_pos);
        if (value_start != std::string::npos) {
            size_t value_end = buf.find("\r\n", value_start);
            int content_length = std::atoi(buf.substr(value_start, value_end - value_start).c_str());
            size_t body_start = header_end + 4;
            size_t body_received = buf.size() - body_start;
            // Ajout du log de debug
            printf("[isRequestComplete] fd %d: Content-Length=%d, body_received=%zu\n", client_fd, content_length, body_received);
            printf("[isRequestComplete] fd %d: body=\"%s\"\n", client_fd, buf.substr(body_start, body_received).c_str());
            if (body_received > (size_t)server.client_max_body_size) {
                printf("[RequestBufferManager] Body size exceeds client_max_body_size\n");
                return false;
            }
            return body_received >= (size_t)content_length;
        }
    }
    // Sinon, requête sans body
    return true;
}