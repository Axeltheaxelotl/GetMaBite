#include "RequestBufferManager.hpp"
#include <cstdlib>
#include <cstdio>
#include <iostream>

RequestBufferManager::RequestBufferManager() {}

RequestBufferManager::~RequestBufferManager() {}

void RequestBufferManager::append(int client_fd, const std::string& data) {
    std::string& buffer = _buffers[client_fd];
    // More conservative memory reservation to reduce memset overhead
    if (buffer.capacity() < buffer.size() + data.size() + 1024) {
        buffer.reserve(buffer.size() + data.size() + 16384); // Reserve extra 16KB instead of 64KB
    }
    buffer += data;
    // Invalidate cache when new data arrives
    invalidateCache(client_fd);
}

std::string RequestBufferManager::get(int client_fd) {
    std::map<int, std::string>::iterator it = _buffers.find(client_fd);
    if (it != _buffers.end()) {
        return it->second;
    }
    return "";
}

void RequestBufferManager::append(int fd, const char* data, size_t len) {
    std::string& buffer = _buffers[fd];
    // More conservative memory reservation to reduce memset overhead
    if (buffer.capacity() < buffer.size() + len + 1024) {
        buffer.reserve(buffer.size() + len + 16384); // Reserve extra 16KB instead of 64KB
    }
    buffer.append(data, len);
    // Invalidate cache when new data arrives
    invalidateCache(fd);
}

void RequestBufferManager::clear(int client_fd) {
    _buffers.erase(client_fd);
    _parseCache.erase(client_fd);
}

bool RequestBufferManager::isRequestComplete(int client_fd) {
    std::map<int, std::string>::iterator it = _buffers.find(client_fd);
    if (it == _buffers.end()) {
        return false;
    }
    
    const std::string& buffer = it->second;
    RequestParseCache& cache = _parseCache[client_fd];
    
    // If already marked complete, return immediately
    if (cache.isComplete) {
        return true;
    }
    
    // If buffer hasn't grown since last parse and we already parsed, return cached result
    if (buffer.length() == cache.lastParsedSize && cache.lastParsedSize > 0) {
        return cache.isComplete;
    }
    
    // Update last parsed size
    cache.lastParsedSize = buffer.length();
    
    // Check headers only if not already complete
    if (!cache.headersComplete) {
        cache.headersComplete = hasCompleteHeaders(buffer);
        if (!cache.headersComplete) {
            return false;
        }
        
        // Parse encoding type and content length only once
        cache.isChunked = isChunkedEncoding(buffer);
        if (!cache.isChunked) {
            cache.contentLength = getContentLength(buffer);
        }
    }
    
    // Check completion based on encoding type
    if (cache.isChunked) {
        cache.isComplete = isChunkedComplete(buffer);
    } else {
        // Content-Length based completion
        size_t headerEnd = buffer.find("\r\n\r\n");
        if (headerEnd == std::string::npos) {
            return false;
        }
        
        size_t bodyStart = headerEnd + 4;
        size_t currentBodyLength = (buffer.length() > bodyStart) ? buffer.length() - bodyStart : 0;
        
        // If no Content-Length, consider complete after headers
        if (cache.contentLength == 0) {
            cache.isComplete = true;
        } else {
            cache.isComplete = currentBodyLength >= cache.contentLength;
        }
    }
    
    return cache.isComplete;
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
    
    // Use strtoul for proper size_t parsing instead of atoi
    char* endptr;
    unsigned long result = strtoul(lengthStr.c_str(), &endptr, 10);
    
    // Check for parsing errors
    if (*endptr != '\0' || lengthStr.empty()) {
        return 0; // Invalid number
    }
    
    return static_cast<size_t>(result);
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
    // Optimize: Only search the last 10 bytes for chunk terminator
    // since "0\r\n\r\n" is only 5 bytes long
    if (buffer.length() < 5) {
        return false;
    }
    
    size_t searchStart = (buffer.length() > 20) ? buffer.length() - 20 : 0;
    size_t pos = buffer.find("0\r\n\r\n", searchStart);
    return pos != std::string::npos;
}

void RequestBufferManager::invalidateCache(int client_fd) {
    std::map<int, RequestParseCache>::iterator it = _parseCache.find(client_fd);
    if (it != _parseCache.end()) {
        // Only reset completion status, keep parsed headers info if still valid
        RequestParseCache& cache = it->second;
        if (!cache.headersComplete) {
            // Headers not complete yet, reset everything
            cache = RequestParseCache();
        } else {
            // Headers are complete, only reset completion flag
            cache.isComplete = false;
        }
    }
}