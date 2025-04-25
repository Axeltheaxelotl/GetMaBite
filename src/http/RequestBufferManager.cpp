#include "RequestBufferManager.hpp"
#include <cstdlib>

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

bool RequestBufferManager::isRequestComplete(int client_fd) {
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
            size_t total_size = header_end + 4 + content_length;
            return buf.size() >= total_size;
        }
    }
    // Sinon, requête sans body
    return true;
}