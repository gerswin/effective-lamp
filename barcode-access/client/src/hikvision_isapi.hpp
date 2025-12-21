#ifndef HIKVISION_ISAPI_HPP
#define HIKVISION_ISAPI_HPP

#include <string>
#include "common.hpp"

namespace client {

class HikvisionISAPI {
public:
    explicit HikvisionISAPI(const barcode_access::HikvisionConfig& config);
    ~HikvisionISAPI();

    // Initialize CURL
    bool init();

    // Open door (sends pulse to door relay)
    bool openDoor(int door_id);

    // Get door status
    bool getDoorStatus(int door_id);

    // Test connection to controller
    bool testConnection();

    // Get last error
    std::string getLastError() const { return last_error_; }

private:
    std::string buildUrl(const std::string& endpoint);
    bool performRequest(const std::string& url, const std::string& method,
                        const std::string& body = "", std::string* response = nullptr);

    barcode_access::HikvisionConfig config_;
    void* curl_ = nullptr;  // CURL handle
    std::string last_error_;
};

} // namespace client

#endif // HIKVISION_ISAPI_HPP
