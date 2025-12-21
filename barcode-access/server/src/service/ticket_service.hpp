#ifndef TICKET_SERVICE_HPP
#define TICKET_SERVICE_HPP

#include <string>
#include <vector>
#include <optional>
#include "dto/ticket_dto.hpp"
#include "common.hpp"

namespace service {

struct TicketInfo {
    std::string uuid;
    bool used;
    std::string used_at;
    int used_at_door;
    std::string created_at;
};

struct ValidationResult {
    bool granted;
    barcode_access::AccessResult reason;
    std::string uuid;
    int door_id;
};

class TicketService {
public:
    static TicketService& getInstance();

    // CRUD operations
    bool createTicket(const std::string& uuid);
    std::optional<TicketInfo> getTicket(const std::string& uuid);
    std::vector<TicketInfo> getAllTickets(int limit = 100, int offset = 0);
    bool deleteTicket(const std::string& uuid);

    // Validation and usage
    ValidationResult validateAndUseTicket(const std::string& uuid, int door_id);

    // Stats
    int getTotalCount();
    int getUsedCount();
    int getAvailableCount();

private:
    TicketService() = default;
    TicketService(const TicketService&) = delete;
    TicketService& operator=(const TicketService&) = delete;
};

} // namespace service

#endif // TICKET_SERVICE_HPP
