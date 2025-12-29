#ifndef DISCOVERY_CLIENT_HPP
#define DISCOVERY_CLIENT_HPP

#include <string>

namespace client {

class DiscoveryClient {
public:
    DiscoveryClient(int discoveryPort = 8888);
    
    // Finds the server URL. Returns empty string if not found.
    // timeoutMs: how long to wait for a response in milliseconds
    std::string findServer(int timeoutMs = 2000);

private:
    int discoveryPort_;
};

} // namespace client

#endif // DISCOVERY_CLIENT_HPP
