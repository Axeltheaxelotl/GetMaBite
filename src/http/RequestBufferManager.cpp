#include "RequestBufferManager.hpp"
#include <cstdlib>
#include <cstdio>
#include <iostream>

RequestBufferManager::RequestBufferManager() {}

RequestBufferManager::~RequestBufferManager() {}

void RequestBufferManager::append(int client_fd, const std::string& data) {
    _buffers[client_fd] += data;
}

std::string RequestBufferManager::get(int client_fd) {
    std::map<int, std::string>::iterator it = _buffers.find(client_fd);
    if (it != _buffers.end()) {
        return it->second;
    }
    return "";
}

void RequestBufferManager::clear(int client_fd) {
    _buffers.erase(client_fd);
}

bool RequestBufferManager::isRequestComplete(int client_fd) {
    std::map<int, std::string>::iterator it = _buffers.find(client_fd);
    if (it == _buffers.end()) {
        return false;
    }
    
    const std::string& buffer = it->second;
    
    // Vérifier si on a les headers complets
    if (!hasCompleteHeaders(buffer)) {
        return false;
    }
    
    // Vérifier si c'est du chunked encoding
    if (isChunkedEncoding(buffer)) {
        return isChunkedComplete(buffer);
    }
    
    // Vérifier avec Content-Length
    size_t contentLength = getContentLength(buffer);
    size_t headerEnd = buffer.find("\r\n\r\n");
    if (headerEnd == std::string::npos) {
        return false;
    }
    
    size_t bodyStart = headerEnd + 4;
    size_t currentBodyLength = buffer.length() - bodyStart;
    
    return currentBodyLength >= contentLength;
}

size_t RequestBufferManager::getBufferSize(int client_fd) {
    std::map<int, std::string>::iterator it = _buffers.find(client_fd);
    if (it != _buffers.end()) {
        return it->second.length();
    }
    return 0;
}

bool RequestBufferManager::hasCompleteHeaders(const std::string& buffer) {
    return buffer.find("\r\n\r\n") != std::string::npos;
}

size_t RequestBufferManager::getContentLength(const std::string& buffer) {
    size_t pos = buffer.find("Content-Length:");
    if (pos == std::string::npos) {
        pos = buffer.find("content-length:");
    }
    if (pos == std::string::npos) {
        return 0;
    }
    
    pos += 15; // longueur de "Content-Length:"
    size_t end = buffer.find("\r\n", pos);
    if (end == std::string::npos) {
        return 0;
    }
    
    std::string lengthStr = buffer.substr(pos, end - pos);
    // Supprimer les espaces
    lengthStr.erase(0, lengthStr.find_first_not_of(" \t"));
    lengthStr.erase(lengthStr.find_last_not_of(" \t") + 1);
    
    return static_cast<size_t>(atoi(lengthStr.c_str()));
}

bool RequestBufferManager::isChunkedEncoding(const std::string& buffer) {
    size_t pos = buffer.find("Transfer-Encoding:");
    if (pos == std::string::npos) {
        pos = buffer.find("transfer-encoding:");
    }
    if (pos == std::string::npos) {
        return false;
    }
    
    size_t end = buffer.find("\r\n", pos);
    if (end == std::string::npos) {
        return false;
    }
    
    std::string encoding = buffer.substr(pos, end - pos);
    return encoding.find("chunked") != std::string::npos;
}

bool RequestBufferManager::isChunkedComplete(const std::string& buffer) {
    // Pour les chunks, on cherche la fin marquée par "0\r\n\r\n"
    return buffer.find("0\r\n\r\n") != std::string::npos;
}