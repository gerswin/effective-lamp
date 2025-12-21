#ifndef BARCODE_ACCESS_COMMON_HPP
#define BARCODE_ACCESS_COMMON_HPP

#include <string>
#include <chrono>
#include <regex>

namespace barcode_access {

// UUID validation regex
inline bool is_valid_uuid(const std::string& uuid) {
    static const std::regex uuid_regex(
        "^[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}$"
    );
    return std::regex_match(uuid, uuid_regex);
}

// Get current timestamp as ISO string
inline std::string get_current_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    char buffer[32];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", std::localtime(&time_t_now));
    return std::string(buffer);
}

// Access result enum
enum class AccessResult {
    GRANTED,
    DENIED_NOT_FOUND,
    DENIED_ALREADY_USED,
    DENIED_INVALID_UUID,
    ERROR_DB,
    ERROR_DOOR
};

inline std::string access_result_to_string(AccessResult result) {
    switch (result) {
        case AccessResult::GRANTED: return "GRANTED";
        case AccessResult::DENIED_NOT_FOUND: return "DENIED_NOT_FOUND";
        case AccessResult::DENIED_ALREADY_USED: return "DENIED_ALREADY_USED";
        case AccessResult::DENIED_INVALID_UUID: return "DENIED_INVALID_UUID";
        case AccessResult::ERROR_DB: return "ERROR_DB";
        case AccessResult::ERROR_DOOR: return "ERROR_DOOR";
        default: return "UNKNOWN";
    }
}

// Configuration structures
struct DatabaseConfig {
    std::string host = "localhost";
    int port = 5432;
    std::string database = "barcode_access";
    std::string user = "access_user";
    std::string password = "";
};

struct HikvisionConfig {
    std::string host = "192.168.1.100";
    int port = 80;
    std::string username = "admin";
    std::string password = "";
    int door_id = 1;  // 1-4 for DS-K2604T
};

struct ServerConfig {
    std::string bind_address = "0.0.0.0";
    int port = 8080;
    DatabaseConfig db;
};

struct ClientConfig {
    int door_id = 1;
    std::string input_device = "/dev/input/event0";
    DatabaseConfig db;
    HikvisionConfig hikvision;
};

} // namespace barcode_access

#endif // BARCODE_ACCESS_COMMON_HPP
