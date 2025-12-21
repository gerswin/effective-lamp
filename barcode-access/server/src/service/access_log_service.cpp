#include "service/access_log_service.hpp"
#include "service/ticket_service.hpp"
#include "db/database.hpp"
#include <iostream>

namespace service {

AccessLogService& AccessLogService::getInstance() {
    static AccessLogService instance;
    return instance;
}

bool AccessLogService::logAccess(const std::string& uuid, int door_id, bool granted,
                                  barcode_access::AccessResult reason) {
    auto& db = db::Database::getInstance();

    std::string reason_str = barcode_access::access_result_to_string(reason);

    db::PGResultGuard result(db.executeParams(
        "INSERT INTO access_logs (ticket_uuid, door_id, granted, attempts, reason) "
        "VALUES ($1, $2, $3, 1, $4)",
        {uuid, std::to_string(door_id), granted ? "TRUE" : "FALSE", reason_str}
    ));

    if (!result.ok()) {
        std::cerr << "Failed to log access for: " << uuid << std::endl;
        return false;
    }

    return true;
}

bool AccessLogService::incrementAttempts(const std::string& uuid, int door_id) {
    auto& db = db::Database::getInstance();

    // Find the most recent log entry for this ticket and door, and increment attempts
    db::PGResultGuard result(db.executeParams(
        "UPDATE access_logs SET attempts = attempts + 1 "
        "WHERE id = (SELECT id FROM access_logs WHERE ticket_uuid = $1 AND door_id = $2 "
        "ORDER BY scanned_at DESC LIMIT 1)",
        {uuid, std::to_string(door_id)}
    ));

    return result.ok();
}

std::vector<AccessLogEntry> AccessLogService::getAllLogs(int limit, int offset) {
    std::vector<AccessLogEntry> logs;
    auto& db = db::Database::getInstance();

    db::PGResultGuard result(db.executeParams(
        "SELECT id, ticket_uuid, door_id, granted, attempts, reason, scanned_at "
        "FROM access_logs ORDER BY scanned_at DESC LIMIT $1 OFFSET $2",
        {std::to_string(limit), std::to_string(offset)}
    ));

    if (!result.ok()) {
        return logs;
    }

    for (int i = 0; i < result.rowCount(); ++i) {
        AccessLogEntry entry;
        entry.id = std::stoll(result.getValue(i, 0));
        entry.ticket_uuid = result.getValue(i, 1);
        entry.door_id = std::stoi(result.getValue(i, 2));
        entry.granted = result.getValue(i, 3) == "t";
        entry.attempts = std::stoi(result.getValue(i, 4));
        entry.reason = result.getValue(i, 5);
        entry.scanned_at = result.getValue(i, 6);
        logs.push_back(entry);
    }

    return logs;
}

std::vector<AccessLogEntry> AccessLogService::getLogsByDoor(int door_id, int limit, int offset) {
    std::vector<AccessLogEntry> logs;
    auto& db = db::Database::getInstance();

    db::PGResultGuard result(db.executeParams(
        "SELECT id, ticket_uuid, door_id, granted, attempts, reason, scanned_at "
        "FROM access_logs WHERE door_id = $1 ORDER BY scanned_at DESC LIMIT $2 OFFSET $3",
        {std::to_string(door_id), std::to_string(limit), std::to_string(offset)}
    ));

    if (!result.ok()) {
        return logs;
    }

    for (int i = 0; i < result.rowCount(); ++i) {
        AccessLogEntry entry;
        entry.id = std::stoll(result.getValue(i, 0));
        entry.ticket_uuid = result.getValue(i, 1);
        entry.door_id = std::stoi(result.getValue(i, 2));
        entry.granted = result.getValue(i, 3) == "t";
        entry.attempts = std::stoi(result.getValue(i, 4));
        entry.reason = result.getValue(i, 5);
        entry.scanned_at = result.getValue(i, 6);
        logs.push_back(entry);
    }

    return logs;
}

std::vector<AccessLogEntry> AccessLogService::getLogsByTicket(const std::string& uuid) {
    std::vector<AccessLogEntry> logs;
    auto& db = db::Database::getInstance();

    db::PGResultGuard result(db.executeParams(
        "SELECT id, ticket_uuid, door_id, granted, attempts, reason, scanned_at "
        "FROM access_logs WHERE ticket_uuid = $1 ORDER BY scanned_at DESC",
        {uuid}
    ));

    if (!result.ok()) {
        return logs;
    }

    for (int i = 0; i < result.rowCount(); ++i) {
        AccessLogEntry entry;
        entry.id = std::stoll(result.getValue(i, 0));
        entry.ticket_uuid = result.getValue(i, 1);
        entry.door_id = std::stoi(result.getValue(i, 2));
        entry.granted = result.getValue(i, 3) == "t";
        entry.attempts = std::stoi(result.getValue(i, 4));
        entry.reason = result.getValue(i, 5);
        entry.scanned_at = result.getValue(i, 6);
        logs.push_back(entry);
    }

    return logs;
}

int AccessLogService::getAttemptCount(const std::string& uuid) {
    auto& db = db::Database::getInstance();

    db::PGResultGuard result(db.executeParams(
        "SELECT COALESCE(SUM(attempts), 0) FROM access_logs WHERE ticket_uuid = $1",
        {uuid}
    ));

    if (!result.ok() || result.rowCount() == 0) {
        return 0;
    }

    return std::stoi(result.getValue(0, 0));
}

int AccessLogService::getTotalAttempts() {
    auto& db = db::Database::getInstance();
    db::PGResultGuard result(db.execute("SELECT COUNT(*) FROM access_logs"));

    if (!result.ok() || result.rowCount() == 0) {
        return 0;
    }

    return std::stoi(result.getValue(0, 0));
}

int AccessLogService::getGrantedCount() {
    auto& db = db::Database::getInstance();
    db::PGResultGuard result(db.execute("SELECT COUNT(*) FROM access_logs WHERE granted = TRUE"));

    if (!result.ok() || result.rowCount() == 0) {
        return 0;
    }

    return std::stoi(result.getValue(0, 0));
}

int AccessLogService::getDeniedCount() {
    auto& db = db::Database::getInstance();
    db::PGResultGuard result(db.execute("SELECT COUNT(*) FROM access_logs WHERE granted = FALSE"));

    if (!result.ok() || result.rowCount() == 0) {
        return 0;
    }

    return std::stoi(result.getValue(0, 0));
}

std::vector<DoorStats> AccessLogService::getDoorStats() {
    std::vector<DoorStats> stats;
    auto& db = db::Database::getInstance();

    db::PGResultGuard result(db.execute(
        "SELECT door_id, COUNT(*) as total, "
        "SUM(CASE WHEN granted THEN 1 ELSE 0 END) as granted, "
        "SUM(CASE WHEN NOT granted THEN 1 ELSE 0 END) as denied "
        "FROM access_logs GROUP BY door_id ORDER BY door_id"
    ));

    if (!result.ok()) {
        return stats;
    }

    for (int i = 0; i < result.rowCount(); ++i) {
        DoorStats ds;
        ds.door_id = std::stoi(result.getValue(i, 0));
        ds.total_attempts = std::stoi(result.getValue(i, 1));
        ds.granted = std::stoi(result.getValue(i, 2));
        ds.denied = std::stoi(result.getValue(i, 3));
        stats.push_back(ds);
    }

    return stats;
}

OverallStats AccessLogService::getOverallStats() {
    OverallStats stats;

    auto& ticketService = TicketService::getInstance();
    stats.total_tickets = ticketService.getTotalCount();
    stats.used_tickets = ticketService.getUsedCount();
    stats.available_tickets = ticketService.getAvailableCount();

    stats.total_access_attempts = getTotalAttempts();
    stats.granted_access = getGrantedCount();
    stats.denied_access = getDeniedCount();
    stats.door_stats = getDoorStats();

    return stats;
}

} // namespace service
