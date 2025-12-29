#ifndef DISCOVERY_SERVICE_HPP
#define DISCOVERY_SERVICE_HPP

#include <string>
#include <thread>
#include <atomic>
#include <functional>

namespace service {

class DiscoveryService {
public:
    DiscoveryService(int discoveryPort, int serverPort);
    ~DiscoveryService();

    // Start the UDP listener in a background thread
    void start();

    // Stop the listener
    void stop();

private:
    void run();

    int discoveryPort_;
    int serverPort_;
    std::atomic<bool> running_;
    std::thread thread_;
    int socketFd_;
};

} // namespace service

#endif // DISCOVERY_SERVICE_HPP
