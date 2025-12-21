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

class AccessClient {
public:
    explicit AccessClient(const std::string& server_url);
    ~AccessClient();

    // Initialize CURL
    bool init();

    // Validate ticket with central server
    ValidationResponse validateTicket(const std::string& uuid, int door_id);

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
