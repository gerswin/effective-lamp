#ifndef ACCESS_LOG_SERVICE_HPP
#define ACCESS_LOG_SERVICE_HPP

#include <string>
#include <vector>
#include "common.hpp"

namespace service {

struct AccessLogEntry {
    int64_t id;
    std::string ticket_uuid;
    int door_id;
    bool granted;
    int attempts;
    std::string reason;
    std::string scanned_at;
};

struct DoorStats {
    int door_id;
    int total_attempts;
    int granted;
    int denied;
};

struct OverallStats {
    int total_tickets;
    int used_tickets;
    int available_tickets;
    int total_access_attempts;
    int granted_access;
    int denied_access;
    std::vector<DoorStats> door_stats;
};

class AccessLogService {
public:
    static AccessLogService& getInstance();

    // Log access attempt
    bool logAccess(const std::string& uuid, int door_id, bool granted,
                   barcode_access::AccessResult reason);

    // Increment attempts for a denied ticket
    bool incrementAttempts(const std::string& uuid, int door_id);

    // Get logs
    std::vector<AccessLogEntry> getAllLogs(int limit = 100, int offset = 0);
    std::vector<AccessLogEntry> getLogsByDoor(int door_id, int limit = 100, int offset = 0);
    std::vector<AccessLogEntry> getLogsByTicket(const std::string& uuid);

    // Get attempt count for a specific ticket
    int getAttemptCount(const std::string& uuid);

    // Stats
    int getTotalAttempts();
    int getGrantedCount();
    int getDeniedCount();
    std::vector<DoorStats> getDoorStats();
    OverallStats getOverallStats();

private:
    AccessLogService() = default;
    AccessLogService(const AccessLogService&) = delete;
    AccessLogService& operator=(const AccessLogService&) = delete;
};

} // namespace service

#endif // ACCESS_LOG_SERVICE_HPP
