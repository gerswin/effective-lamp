#ifndef DB_CLIENT_HPP
#define DB_CLIENT_HPP

#include <string>
#include "common.hpp"

namespace client {

struct ValidationResponse {
    bool granted;
    std::string reason;
    std::string uuid;
    int door_id;
};

struct ProvisionConfig {
    int door_id;
    std::string hik_host;
    int hik_port;
    std::string hik_user;
    std::string hik_password;
    bool success;
};

class AccessClient {
public:
    explicit AccessClient(const std::string& server_url);
    ~AccessClient();

    // Initialize CURL
    bool init();

    // Validate ticket with central server
    ValidationResponse validateTicket(const std::string& uuid, int door_id);

    // Rollback usage if door failed to open
    bool rollbackUsage(const std::string& uuid, int door_id);

    // Auto-provision client configuration
    ProvisionConfig provision(const std::string& hardwareId);

    // Test connection to server
    bool testConnection();

    // Get last error
    std::string getLastError() const { return last_error_; }

private:
    bool performRequest(const std::string& url, const std::string& method,
                        const std::string& body, std::string* response);

    std::string server_url_;
    void* curl_ = nullptr;
    std::string last_error_;
};

} // namespace client

#endif // DB_CLIENT_HPP
