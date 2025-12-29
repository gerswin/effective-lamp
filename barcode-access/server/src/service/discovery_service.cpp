#include "discovery_service.hpp"
#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>

namespace service {

DiscoveryService::DiscoveryService(int discoveryPort, int serverPort)
    : discoveryPort_(discoveryPort), serverPort_(serverPort), running_(false), socketFd_(-1) {}

DiscoveryService::~DiscoveryService() {
    stop();
}

void DiscoveryService::start() {
    if (running_) return;
    running_ = true;
    thread_ = std::thread(&DiscoveryService::run, this);
}

void DiscoveryService::stop() {
    if (!running_) return;
    running_ = false;
    
    // Close socket to unblock recvfrom
    if (socketFd_ >= 0) {
        close(socketFd_);
        socketFd_ = -1;
    }

    if (thread_.joinable()) {
        thread_.join();
    }
}

void DiscoveryService::run() {
    std::cout << "Discovery Service: Starting on UDP port " << discoveryPort_ << std::endl;

    socketFd_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (socketFd_ < 0) {
        std::cerr << "Discovery Service: Failed to create socket" << std::endl;
        return;
    }

    // Set SO_REUSEADDR to allow restarting quickly
    int opt = 1;
    setsockopt(socketFd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(discoveryPort_);

    if (bind(socketFd_, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Discovery Service: Failed to bind to port " << discoveryPort_ << std::endl;
        close(socketFd_);
        socketFd_ = -1;
        return;
    }

    char buffer[1024];
    sockaddr_in clientAddr{};
    socklen_t clientLen = sizeof(clientAddr);

    while (running_) {
        // Clear buffer
        memset(buffer, 0, sizeof(buffer));

        // Receive message
        ssize_t n = recvfrom(socketFd_, buffer, sizeof(buffer) - 1, 0,
                             (struct sockaddr*)&clientAddr, &clientLen);

        if (n < 0) {
            if (running_) {
                // If running is still true, this is a real error
                // However, close() in stop() causes this too, so check errno if needed
                // std::cerr << "Discovery Service: recvfrom failed" << std::endl;
            }
            break;
        }

        std::string message(buffer);
        // Simple protocol: Client says "WHO_IS_BARCODE_SERVER?"
        if (message.find("WHO_IS_BARCODE_SERVER?") != std::string::npos) {
            std::string response = "IAM_BARCODE_SERVER|" + std::to_string(serverPort_);
            
            // Log discovery request (verbose)
            // std::cout << "Discovery: Request from " << inet_ntoa(clientAddr.sin_addr) << std::endl;

            sendto(socketFd_, response.c_str(), response.length(), 0,
                   (struct sockaddr*)&clientAddr, clientLen);
        }
    }

    std::cout << "Discovery Service: Stopped" << std::endl;
}

} // namespace service
