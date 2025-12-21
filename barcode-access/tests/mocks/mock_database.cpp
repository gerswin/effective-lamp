#include "mock_database.hpp"
#include <chrono>
#include <ctime>
#include <algorithm>

namespace mocks {

MockDatabase& MockDatabase::getInstance() {
    static MockDatabase instance;
    return instance;
}

void MockDatabase::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    tickets_.clear();
    access_logs_.clear();
    next_log_id_ = 1;
}

static std::string getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    char buffer[32];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", std::localtime(&time_t_now));
    return std::string(buffer);
}

bool MockDatabase::createTicket(const std::string& uuid) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (tickets_.find(uuid) != tickets_.end()) {
        return true; // Already exists, like ON CONFLICT DO NOTHING
    }

    MockTicket ticket;
    ticket.uuid = uuid;
    ticket.used = false;
    ticket.created_at = getCurrentTimestamp();

    tickets_[uuid] = ticket;
    return true;
}

std::optional<MockTicket> MockDatabase::getTicket(const std::string& uuid) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = tickets_.find(uuid);
    if (it == tickets_.end()) {
        return std::nullopt;
    }
    return it->second;
}

std::vector<MockTicket> MockDatabase::getAllTickets(int limit, int offset) {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<MockTicket> result;
    int count = 0;
    int skipped = 0;

    for (const auto& pair : tickets_) {
        if (skipped < offset) {
            skipped++;
            continue;
        }
        if (count >= limit) break;
        result.push_back(pair.second);
        count++;
    }

    return result;
}

bool MockDatabase::deleteTicket(const std::string& uuid) {
    std::lock_guard<std::mutex> lock(mutex_);
    return tickets_.erase(uuid) > 0;
}

bool MockDatabase::markTicketUsed(const std::string& uuid, int door_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = tickets_.find(uuid);
    if (it == tickets_.end()) {
        return false;
    }

    it->second.used = true;
    it->second.used_at = getCurrentTimestamp();
    it->second.used_at_door = door_id;
    return true;
}

int MockDatabase::getTotalTickets() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return static_cast<int>(tickets_.size());
}

int MockDatabase::getUsedTickets() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return static_cast<int>(std::count_if(tickets_.begin(), tickets_.end(),
        [](const auto& pair) { return pair.second.used; }));
}

int MockDatabase::getAvailableTickets() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return static_cast<int>(std::count_if(tickets_.begin(), tickets_.end(),
        [](const auto& pair) { return !pair.second.used; }));
}

bool MockDatabase::logAccess(const std::string& uuid, int door_id, bool granted, const std::string& reason) {
    std::lock_guard<std::mutex> lock(mutex_);

    MockAccessLog log;
    log.id = next_log_id_++;
    log.ticket_uuid = uuid;
    log.door_id = door_id;
    log.granted = granted;
    log.attempts = 1;
    log.reason = reason;
    log.scanned_at = getCurrentTimestamp();

    access_logs_.push_back(log);
    return true;
}

std::vector<MockAccessLog> MockDatabase::getAllLogs(int limit, int offset) {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<MockAccessLog> result;
    int count = 0;

    // Return in reverse order (newest first)
    for (auto it = access_logs_.rbegin(); it != access_logs_.rend(); ++it) {
        if (offset > 0) {
            offset--;
            continue;
        }
        if (count >= limit) break;
        result.push_back(*it);
        count++;
    }

    return result;
}

std::vector<MockAccessLog> MockDatabase::getLogsByDoor(int door_id, int limit) {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<MockAccessLog> result;
    int count = 0;

    for (auto it = access_logs_.rbegin(); it != access_logs_.rend(); ++it) {
        if (it->door_id == door_id) {
            if (count >= limit) break;
            result.push_back(*it);
            count++;
        }
    }

    return result;
}

std::vector<MockAccessLog> MockDatabase::getLogsByTicket(const std::string& uuid) {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<MockAccessLog> result;

    for (auto it = access_logs_.rbegin(); it != access_logs_.rend(); ++it) {
        if (it->ticket_uuid == uuid) {
            result.push_back(*it);
        }
    }

    return result;
}

int MockDatabase::getAttemptCount(const std::string& uuid) {
    std::lock_guard<std::mutex> lock(mutex_);

    int count = 0;
    for (const auto& log : access_logs_) {
        if (log.ticket_uuid == uuid) {
            count += log.attempts;
        }
    }
    return count;
}

int MockDatabase::getTotalAttempts() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return static_cast<int>(access_logs_.size());
}

int MockDatabase::getGrantedCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return static_cast<int>(std::count_if(access_logs_.begin(), access_logs_.end(),
        [](const MockAccessLog& log) { return log.granted; }));
}

int MockDatabase::getDeniedCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return static_cast<int>(std::count_if(access_logs_.begin(), access_logs_.end(),
        [](const MockAccessLog& log) { return !log.granted; }));
}

} // namespace mocks
