#include "discovery_client.hpp"
#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <vector>
#include <sstream>

namespace client {

DiscoveryClient::DiscoveryClient(int discoveryPort)
    : discoveryPort_(discoveryPort) {}

std::string DiscoveryClient::findServer(int timeoutMs) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        std::cerr << "Discovery Client: Failed to create socket" << std::endl;
        return "";
    }

    // Enable broadcast
    int broadcast = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) < 0) {
        std::cerr << "Discovery Client: Failed to enable broadcast" << std::endl;
        close(sock);
        return "";
    }

    // Set timeout
    struct timeval tv;
    tv.tv_sec = timeoutMs / 1000;
    tv.tv_usec = (timeoutMs % 1000) * 1000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

    sockaddr_in broadcastAddr{};
    broadcastAddr.sin_family = AF_INET;
    broadcastAddr.sin_port = htons(discoveryPort_);
    broadcastAddr.sin_addr.s_addr = INADDR_BROADCAST;

    std::string message = "WHO_IS_BARCODE_SERVER?";
    if (sendto(sock, message.c_str(), message.length(), 0,
               (struct sockaddr*)&broadcastAddr, sizeof(broadcastAddr)) < 0) {
        std::cerr << "Discovery Client: Failed to send broadcast" << std::endl;
        close(sock);
        return "";
    }

    std::cout << "Discovery: Sending broadcast..." << std::endl;

    char buffer[1024];
    sockaddr_in serverAddr{};
    socklen_t serverLen = sizeof(serverAddr);

    // Try to receive response
    // We might receive our own broadcast, so we loop until we get a valid server response or timeout
    // (Actually, normally we don't receive our own broadcast unless we bind to INADDR_ANY, 
    // but here we didn't bind, just created a socket. 
    // However, if we run client and server on same machine, we might get interference)
    
    while (true) {
        ssize_t n = recvfrom(sock, buffer, sizeof(buffer) - 1, 0,
                             (struct sockaddr*)&serverAddr, &serverLen);

        if (n < 0) {
            // Timeout or error
            break;
        }

        std::string response(buffer, n);
        if (response.find("IAM_BARCODE_SERVER|") == 0) {
            // Found it!
            std::string portStr = response.substr(19); // Length of prefix
            int port = 8080;
            try {
                port = std::stoi(portStr);
            } catch (...) {}

            std::string ip = inet_ntoa(serverAddr.sin_addr);
            close(sock);

            std::stringstream url;
            url << "http://" << ip << ":" << port;
            return url.str();
        }
    }

    close(sock);
    return "";
}

} // namespace client
