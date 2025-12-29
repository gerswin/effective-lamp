#ifndef BARCODE_ACCESS_COMMON_HPP
#define BARCODE_ACCESS_COMMON_HPP

#include <string>
#include <chrono>
#include <regex>

#include <random>

namespace barcode_access {

// NanoID validation (max 10 chars, safe characters)
inline bool is_valid_ticket_code(const std::string& code) {
    if (code.length() > 10 || code.empty()) return false;
    static const std::string charset = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz-_";
    return code.find_first_not_of(charset) == std::string::npos;
}

// Simple NanoID generator (length 10)
inline std::string generate_nanoid(int length = 10) {
    static const char charset[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz-_";
    std::random_device rd;
    std::mt19937 generator(rd());
    std::uniform_int_distribution<int> distribution(0, sizeof(charset) - 2); // -2 because sizeof includes null terminator

    std::string result;
    result.reserve(length);
    for (int i = 0; i < length; ++i) {
        result += charset[distribution(generator)];
    }
    return result;
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
    DENIED_MAX_USES_REACHED,
    DENIED_INVALID_FORMAT,
    ERROR_DB,
    ERROR_DOOR,
    ROLLBACK
};

inline std::string access_result_to_string(AccessResult result) {
    switch (result) {
        case AccessResult::GRANTED: return "GRANTED";
        case AccessResult::DENIED_NOT_FOUND: return "DENIED_NOT_FOUND";
        case AccessResult::DENIED_ALREADY_USED: return "DENIED_ALREADY_USED";
        case AccessResult::DENIED_MAX_USES_REACHED: return "DENIED_MAX_USES_REACHED";
        case AccessResult::DENIED_INVALID_FORMAT: return "DENIED_INVALID_FORMAT";
        case AccessResult::ERROR_DB: return "ERROR_DB";
        case AccessResult::ERROR_DOOR: return "ERROR_DOOR";
        case AccessResult::ROLLBACK: return "ROLLBACK";
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
