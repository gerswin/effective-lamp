#ifndef MOCK_DATABASE_HPP
#define MOCK_DATABASE_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include <mutex>
#include "common.hpp"
#include "dto/access_log_dto.hpp"

namespace mocks {

struct MockAccessLog {
    int64_t id;
    std::string ticket_uuid;
    int door_id;
    bool granted;
    int attempts;
    std::string reason;
    std::string scanned_at;
};

struct MockTicket {
    std::string uuid;
    bool used = false;
    int max_uses = -1; // -1 for unlimited uses
    int current_uses = 0;
    std::string used_at;
    int used_at_door = 0;
    std::string created_at;
};

class MockDatabase {
public:
    static MockDatabase& getInstance();

    // Reset all data (call between tests)
    void reset();

    // Ticket operations
    bool createTicket(const std::string& uuid, int max_uses = -1);
    std::optional<MockTicket> getTicket(const std::string& uuid);
    std::vector<MockTicket> getAllTickets(int limit = 100, int offset = 0);
    bool deleteTicket(const std::string& uuid);
    bool markTicketUsed(const std::string& uuid, int door_id);

    // Ticket stats
    int getTotalTickets() const;
    int getUsedTickets() const;
    int getAvailableTickets() const;

    // Access log operations
    bool logAccess(const std::string& uuid, int door_id, bool granted, const std::string& reason);
    std::vector<MockAccessLog> getAllLogs(int limit = 100, int offset = 0);
    std::vector<MockAccessLog> getLogsByDoor(int door_id, int limit = 100);
    std::vector<MockAccessLog> getLogsByTicket(const std::string& uuid);
    int getAttemptCount(const std::string& uuid);

    // Access log stats
    int getTotalAttempts() const;
    int getGrantedCount() const;
    int getDeniedCount() const;

private:
    MockDatabase() = default;
    MockDatabase(const MockDatabase&) = delete;
    MockDatabase& operator=(const MockDatabase&) = delete;

    std::unordered_map<std::string, MockTicket> tickets_;
    std::vector<MockAccessLog> access_logs_;
    int64_t next_log_id_ = 1;
    mutable std::mutex mutex_;
};

} // namespace mocks

#endif // MOCK_DATABASE_HPP
