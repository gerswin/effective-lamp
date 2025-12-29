#include "service/ticket_service.hpp"
#include "db/database.hpp"
#include <iostream>

namespace service {

TicketService& TicketService::getInstance() {
    static TicketService instance;
    return instance;
}

bool TicketService::createTicket(const std::string& uuid, int max_uses) {
    if (!barcode_access::is_valid_ticket_code(uuid)) {
        std::cerr << "Invalid Ticket Code format: " << uuid << std::endl;
        return false;
    }

    auto& db = db::Database::getInstance();
    db::PGResultGuard result(db.executeParams(
        "INSERT INTO tickets (uuid, max_uses) VALUES ($1, $2) ON CONFLICT (uuid) DO NOTHING",
        {uuid, std::to_string(max_uses)}
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
        "SELECT uuid, used, max_uses, current_uses, used_at, used_at_door, created_at FROM tickets WHERE uuid = $1",
        {uuid}
    ));

    if (!result.ok() || result.rowCount() == 0) {
        return std::nullopt;
    }

    TicketInfo ticket;
    ticket.uuid = result.getValue(0, 0);
    // ticket.used = result.getValue(0, 1) == "t"; // Old way
    ticket.max_uses = std::stoi(result.getValue(0, 2));
    ticket.current_uses = std::stoi(result.getValue(0, 3));

    // Determine 'used' status based on new logic
    if (ticket.max_uses != -1) { // Limited ticket
        ticket.used = (ticket.current_uses >= ticket.max_uses);
    } else { // Unlimited ticket
        ticket.used = false; // Unlimited tickets are never "used up"
    }

    ticket.used_at = result.getValue(0, 4);
    ticket.used_at_door = result.getValue(0, 5).empty() ? 0 : std::stoi(result.getValue(0, 5));
    ticket.created_at = result.getValue(0, 6);

    return ticket;
}

std::vector<TicketInfo> TicketService::getAllTickets(int limit, int offset, const std::string& filter, const std::string& search) {
    std::vector<TicketInfo> tickets;
    auto& db = db::Database::getInstance();

    std::string query = "SELECT uuid, used, max_uses, current_uses, used_at, used_at_door, created_at FROM tickets WHERE 1=1";
    std::vector<std::string> params;
    int param_idx = 1;

    if (filter == "used") {
        query += " AND (used = TRUE OR (max_uses != -1 AND current_uses >= max_uses))";
    } else if (filter == "available") {
        query += " AND (used = FALSE AND (max_uses = -1 OR current_uses < max_uses))";
    }

    if (!search.empty()) {
        query += " AND uuid::text ILIKE $" + std::to_string(param_idx++);
        params.push_back("%" + search + "%");
    }

    query += " ORDER BY created_at DESC LIMIT $" + std::to_string(param_idx++);
    params.push_back(std::to_string(limit));

    query += " OFFSET $" + std::to_string(param_idx++);
    params.push_back(std::to_string(offset));

    db::PGResultGuard result(db.executeParams(query, params));

    if (!result.ok()) {
        return tickets;
    }

    for (int i = 0; i < result.rowCount(); ++i) {
        TicketInfo ticket;
        ticket.uuid = result.getValue(i, 0);
        // ticket.used = result.getValue(i, 1) == "t"; // Old way
        ticket.max_uses = std::stoi(result.getValue(i, 2));
        ticket.current_uses = std::stoi(result.getValue(i, 3));

        // Determine 'used' status based on new logic
        if (ticket.max_uses != -1) { // Limited ticket
            ticket.used = (ticket.current_uses >= ticket.max_uses);
        } else { // Unlimited ticket
            ticket.used = false; // Unlimited tickets are never "used up"
        }

        ticket.used_at = result.getValue(i, 4);
        ticket.used_at_door = result.getValue(i, 5).empty() ? 0 : std::stoi(result.getValue(i, 5));
        ticket.created_at = result.getValue(i, 6);
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
    vr.max_uses = -1; // Default values
    vr.current_uses = 0;

    // Validate Ticket Code format
    if (!barcode_access::is_valid_ticket_code(uuid)) {
        vr.granted = false;
        vr.reason = barcode_access::AccessResult::DENIED_INVALID_FORMAT;
        return vr;
    }

    auto& db = db::Database::getInstance();

    // Start transaction for atomic check-and-update
    db.beginTransaction();

    // Check if ticket exists and get its status, max_uses, and current_uses
    db::PGResultGuard checkResult(db.executeParams(
        "SELECT used, max_uses, current_uses FROM tickets WHERE uuid = $1 FOR UPDATE",
        {uuid}
    ));

    if (!checkResult.ok() || checkResult.rowCount() == 0) {
        db.rollbackTransaction();
        vr.granted = false;
        vr.reason = barcode_access::AccessResult::DENIED_NOT_FOUND;
        return vr;
    }

    int max_uses = std::stoi(checkResult.getValue(0, 1));
    int current_uses = std::stoi(checkResult.getValue(0, 2));

    vr.max_uses = max_uses;
    vr.current_uses = current_uses;

    // Check against max_uses
    if (max_uses != -1 && current_uses >= max_uses) {
        db.rollbackTransaction();
        vr.granted = false;
        vr.reason = barcode_access::AccessResult::DENIED_MAX_USES_REACHED;
        // Optionally update 'used' to true here if it wasn't already for some reason
        db.executeParams("UPDATE tickets SET used = TRUE WHERE uuid = $1", {uuid}); // Ensure used is true
        return vr;
    }

    // Increment current_uses and potentially mark as fully used
    int new_current_uses = current_uses + 1;
    bool new_used_status = (max_uses != -1 && new_current_uses >= max_uses);

    db::PGResultGuard updateResult(db.executeParams(
        "UPDATE tickets SET used = $1, current_uses = $2, used_at = NOW(), used_at_door = $4 WHERE uuid = $3",
        {new_used_status ? "TRUE" : "FALSE", std::to_string(new_current_uses), uuid, std::to_string(door_id)}
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
    vr.current_uses = new_current_uses; // Update for the response
    return vr;
}

bool TicketService::rollbackUsage(const std::string& uuid, int door_id) {
    if (!barcode_access::is_valid_ticket_code(uuid)) {
        return false;
    }

    auto& db = db::Database::getInstance();

    // Start transaction
    db.beginTransaction();

    // Get current state
    db::PGResultGuard checkResult(db.executeParams(
        "SELECT used, max_uses, current_uses FROM tickets WHERE uuid = $1 FOR UPDATE",
        {uuid}
    ));

    if (!checkResult.ok() || checkResult.rowCount() == 0) {
        db.rollbackTransaction();
        return false;
    }

    int current_uses = std::stoi(checkResult.getValue(0, 2));

    if (current_uses <= 0) {
        // Nothing to rollback
        db.rollbackTransaction();
        return true; 
    }

    int new_current_uses = current_uses - 1;
    
    // We always set used to FALSE if we are rolling back, unless we want to be very smart about max_uses logic.
    // If max_uses was reached, used was TRUE. Now current < max, so used should be FALSE.
    // If unlimited, used was FALSE (or TRUE if we track "at least one use").
    // Let's assume used means "fully used / exhausted".
    
    // Logic from validateAndUse:
    // if (ticket.max_uses != -1) { ticket.used = (ticket.current_uses >= ticket.max_uses); } else { ticket.used = false; }
    
    int max_uses = std::stoi(checkResult.getValue(0, 1));
    bool new_used_status = false;
    if (max_uses != -1) {
        new_used_status = (new_current_uses >= max_uses);
    }

    db::PGResultGuard updateResult(db.executeParams(
        "UPDATE tickets SET used = $1, current_uses = $2 WHERE uuid = $3",
        {new_used_status ? "TRUE" : "FALSE", std::to_string(new_current_uses), uuid}
    ));

    if (!updateResult.ok()) {
        db.rollbackTransaction();
        return false;
    }

    // Also verify if the last log matches this door (optional but good for consistency)
    // For now we just rollback the ticket state.

    db.commitTransaction();
    return true;
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
    // A ticket is considered "used" if it's explicitly marked as used OR if current_uses >= max_uses (and max_uses is not unlimited)
    db::PGResultGuard result(db.execute("SELECT COUNT(*) FROM tickets WHERE used = TRUE OR (max_uses != -1 AND current_uses >= max_uses)"));

    if (!result.ok() || result.rowCount() == 0) {
        return 0;
    }

    return std::stoi(result.getValue(0, 0));
}

int TicketService::getAvailableCount() {
    auto& db = db::Database::getInstance();
    // A ticket is considered "available" if it's NOT explicitly marked as used AND (max_uses is unlimited OR current_uses < max_uses)
    db::PGResultGuard result(db.execute("SELECT COUNT(*) FROM tickets WHERE used = FALSE AND (max_uses = -1 OR current_uses < max_uses)"));

    if (!result.ok() || result.rowCount() == 0) {
        return 0;
    }

    return std::stoi(result.getValue(0, 0));
}

} // namespace service