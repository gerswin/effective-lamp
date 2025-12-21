#include "service/ticket_service.hpp"
#include "db/database.hpp"
#include <iostream>

namespace service {

TicketService& TicketService::getInstance() {
    static TicketService instance;
    return instance;
}

bool TicketService::createTicket(const std::string& uuid) {
    if (!barcode_access::is_valid_uuid(uuid)) {
        std::cerr << "Invalid UUID format: " << uuid << std::endl;
        return false;
    }

    auto& db = db::Database::getInstance();
    db::PGResultGuard result(db.executeParams(
        "INSERT INTO tickets (uuid) VALUES ($1) ON CONFLICT (uuid) DO NOTHING",
        {uuid}
    ));

    if (!result.ok()) {
        std::cerr << "Failed to create ticket: " << uuid << std::endl;
        return false;
    }

    return true;
}

std::optional<TicketInfo> TicketService::getTicket(const std::string& uuid) {
    auto& db = db::Database::getInstance();
    db::PGResultGuard result(db.executeParams(
        "SELECT uuid, used, used_at, used_at_door, created_at FROM tickets WHERE uuid = $1",
        {uuid}
    ));

    if (!result.ok() || result.rowCount() == 0) {
        return std::nullopt;
    }

    TicketInfo ticket;
    ticket.uuid = result.getValue(0, 0);
    ticket.used = result.getValue(0, 1) == "t";
    ticket.used_at = result.getValue(0, 2);
    ticket.used_at_door = result.getValue(0, 3).empty() ? 0 : std::stoi(result.getValue(0, 3));
    ticket.created_at = result.getValue(0, 4);

    return ticket;
}

std::vector<TicketInfo> TicketService::getAllTickets(int limit, int offset) {
    std::vector<TicketInfo> tickets;

    auto& db = db::Database::getInstance();
    db::PGResultGuard result(db.executeParams(
        "SELECT uuid, used, used_at, used_at_door, created_at FROM tickets "
        "ORDER BY created_at DESC LIMIT $1 OFFSET $2",
        {std::to_string(limit), std::to_string(offset)}
    ));

    if (!result.ok()) {
        return tickets;
    }

    for (int i = 0; i < result.rowCount(); ++i) {
        TicketInfo ticket;
        ticket.uuid = result.getValue(i, 0);
        ticket.used = result.getValue(i, 1) == "t";
        ticket.used_at = result.getValue(i, 2);
        ticket.used_at_door = result.getValue(i, 3).empty() ? 0 : std::stoi(result.getValue(i, 3));
        ticket.created_at = result.getValue(i, 4);
        tickets.push_back(ticket);
    }

    return tickets;
}

bool TicketService::deleteTicket(const std::string& uuid) {
    auto& db = db::Database::getInstance();
    db::PGResultGuard result(db.executeParams(
        "DELETE FROM tickets WHERE uuid = $1",
        {uuid}
    ));

    return result.ok();
}

ValidationResult TicketService::validateAndUseTicket(const std::string& uuid, int door_id) {
    ValidationResult vr;
    vr.uuid = uuid;
    vr.door_id = door_id;

    // Validate UUID format
    if (!barcode_access::is_valid_uuid(uuid)) {
        vr.granted = false;
        vr.reason = barcode_access::AccessResult::DENIED_INVALID_UUID;
        return vr;
    }

    auto& db = db::Database::getInstance();

    // Start transaction for atomic check-and-update
    db.beginTransaction();

    // Check if ticket exists and get its status
    db::PGResultGuard checkResult(db.executeParams(
        "SELECT used FROM tickets WHERE uuid = $1 FOR UPDATE",
        {uuid}
    ));

    if (!checkResult.ok() || checkResult.rowCount() == 0) {
        db.rollbackTransaction();
        vr.granted = false;
        vr.reason = barcode_access::AccessResult::DENIED_NOT_FOUND;
        return vr;
    }

    bool already_used = checkResult.getValue(0, 0) == "t";

    if (already_used) {
        db.rollbackTransaction();
        vr.granted = false;
        vr.reason = barcode_access::AccessResult::DENIED_ALREADY_USED;
        return vr;
    }

    // Mark ticket as used
    db::PGResultGuard updateResult(db.executeParams(
        "UPDATE tickets SET used = TRUE, used_at = NOW(), used_at_door = $2 WHERE uuid = $1",
        {uuid, std::to_string(door_id)}
    ));

    if (!updateResult.ok()) {
        db.rollbackTransaction();
        vr.granted = false;
        vr.reason = barcode_access::AccessResult::ERROR_DB;
        return vr;
    }

    db.commitTransaction();

    vr.granted = true;
    vr.reason = barcode_access::AccessResult::GRANTED;
    return vr;
}

int TicketService::getTotalCount() {
    auto& db = db::Database::getInstance();
    db::PGResultGuard result(db.execute("SELECT COUNT(*) FROM tickets"));

    if (!result.ok() || result.rowCount() == 0) {
        return 0;
    }

    return std::stoi(result.getValue(0, 0));
}

int TicketService::getUsedCount() {
    auto& db = db::Database::getInstance();
    db::PGResultGuard result(db.execute("SELECT COUNT(*) FROM tickets WHERE used = TRUE"));

    if (!result.ok() || result.rowCount() == 0) {
        return 0;
    }

    return std::stoi(result.getValue(0, 0));
}

int TicketService::getAvailableCount() {
    auto& db = db::Database::getInstance();
    db::PGResultGuard result(db.execute("SELECT COUNT(*) FROM tickets WHERE used = FALSE"));

    if (!result.ok() || result.rowCount() == 0) {
        return 0;
    }

    return std::stoi(result.getValue(0, 0));
}

} // namespace service
