#ifndef ACCESS_CONTROLLER_HPP
#define ACCESS_CONTROLLER_HPP

#include "oatpp/web/server/api/ApiController.hpp"
#include "oatpp/core/macro/codegen.hpp"
#include "oatpp/core/macro/component.hpp"
#include "dto/ticket_dto.hpp"
#include "dto/access_log_dto.hpp"
#include "service/ticket_service.hpp"
#include "service/access_log_service.hpp"

#include OATPP_CODEGEN_BEGIN(ApiController)

namespace controller {

class AccessController : public oatpp::web::server::api::ApiController {
public:
    AccessController(OATPP_COMPONENT(std::shared_ptr<ObjectMapper>, objectMapper))
        : oatpp::web::server::api::ApiController(objectMapper) {}

    static std::shared_ptr<AccessController> createShared(
        OATPP_COMPONENT(std::shared_ptr<ObjectMapper>, objectMapper)) {
        return std::make_shared<AccessController>(objectMapper);
    }

    // Validate ticket and open door (called by door clients)
    ENDPOINT("POST", "/api/access/validate", validateAccess,
             BODY_DTO(Object<dto::ValidateTicketDto>, validateDto)) {

        auto& ticketService = service::TicketService::getInstance();
        auto& logService = service::AccessLogService::getInstance();

        auto response = dto::ValidateResponseDto::createShared();
        response->uuid = validateDto->uuid;
        response->door_id = validateDto->door_id;
        response->timestamp = barcode_access::get_current_timestamp();

        if (!validateDto->uuid || validateDto->uuid->empty()) {
            response->granted = false;
            response->reason = "UUID is required";
            logService.logAccess("", validateDto->door_id, false,
                                barcode_access::AccessResult::DENIED_INVALID_UUID);
            return createDtoResponse(Status::CODE_400, response);
        }

        std::string uuid = validateDto->uuid;
        int door_id = validateDto->door_id;

        auto result = ticketService.validateAndUseTicket(uuid, door_id);

        response->granted = result.granted;
        response->reason = barcode_access::access_result_to_string(result.reason);
        response->max_uses = result.max_uses;
        response->current_uses = result.current_uses;

        // Log the access attempt
        logService.logAccess(uuid, door_id, result.granted, result.reason);

        if (result.granted) {
            return createDtoResponse(Status::CODE_200, response);
        } else {
            // For denied access, still return 200 but with granted=false
            // This allows the client to handle the response properly
            return createDtoResponse(Status::CODE_200, response);
        }
    }

    // Rollback usage (called by clients if door fails to open)
    ENDPOINT("POST", "/api/access/rollback", rollbackUsage,
             BODY_DTO(Object<dto::RollbackTicketDto>, rollbackDto)) {

        auto& ticketService = service::TicketService::getInstance();
        auto& logService = service::AccessLogService::getInstance();

        auto response = dto::RollbackResponseDto::createShared();

        if (!rollbackDto->uuid || rollbackDto->uuid->empty()) {
            response->success = false;
            response->message = "UUID is required";
            return createDtoResponse(Status::CODE_400, response);
        }

        std::string uuid = rollbackDto->uuid;
        int door_id = rollbackDto->door_id;

        bool result = ticketService.rollbackUsage(uuid, door_id);

        if (result) {
            response->success = true;
            response->message = "Rollback successful";
            // Log the rollback
            logService.logAccess(uuid, door_id, false, barcode_access::AccessResult::ROLLBACK); 
        } else {
            response->success = false;
            response->message = "Rollback failed";
        }

        return createDtoResponse(Status::CODE_200, response);
    }

    // Get access logs
    ENDPOINT("GET", "/api/access/logs", getLogs,
             QUERY(Int32, limit, "limit", 100),
             QUERY(Int32, offset, "offset", 0)) {

        auto& logService = service::AccessLogService::getInstance();
        auto response = dto::AccessLogListResponseDto::createShared();

        auto logs = logService.getAllLogs(limit, offset);

        response->success = true;
        response->total = logService.getTotalAttempts();
        response->granted_count = logService.getGrantedCount();
        response->denied_count = logService.getDeniedCount();

        auto logList = oatpp::List<oatpp::Object<dto::AccessLogDto>>::createShared();
        for (const auto& le : logs) {
            auto log = dto::AccessLogDto::createShared();
            log->id = le.id;
            log->ticket_uuid = le.ticket_uuid;
            log->door_id = le.door_id;
            log->granted = le.granted;
            log->attempts = le.attempts;
            log->reason = le.reason;
            log->scanned_at = le.scanned_at;
            logList->push_back(log);
        }
        response->logs = logList;

        return createDtoResponse(Status::CODE_200, response);
    }

    // Get logs by door
    ENDPOINT("GET", "/api/access/logs/door/{door_id}", getLogsByDoor,
             PATH(Int32, door_id),
             QUERY(Int32, limit, "limit", 100),
             QUERY(Int32, offset, "offset", 0)) {

        auto& logService = service::AccessLogService::getInstance();
        auto response = dto::AccessLogListResponseDto::createShared();

        auto logs = logService.getLogsByDoor(door_id, limit, offset);

        response->success = true;
        response->total = static_cast<int>(logs.size());

        auto logList = oatpp::List<oatpp::Object<dto::AccessLogDto>>::createShared();
        for (const auto& le : logs) {
            auto log = dto::AccessLogDto::createShared();
            log->id = le.id;
            log->ticket_uuid = le.ticket_uuid;
            log->door_id = le.door_id;
            log->granted = le.granted;
            log->attempts = le.attempts;
            log->reason = le.reason;
            log->scanned_at = le.scanned_at;
            logList->push_back(log);
        }
        response->logs = logList;

        return createDtoResponse(Status::CODE_200, response);
    }

    // Get logs by ticket
    ENDPOINT("GET", "/api/access/logs/ticket/{uuid}", getLogsByTicket,
             PATH(String, uuid)) {

        auto& logService = service::AccessLogService::getInstance();
        auto response = dto::AccessLogListResponseDto::createShared();

        auto logs = logService.getLogsByTicket(uuid);

        response->success = true;
        response->total = static_cast<int>(logs.size());

        auto logList = oatpp::List<oatpp::Object<dto::AccessLogDto>>::createShared();
        for (const auto& le : logs) {
            auto log = dto::AccessLogDto::createShared();
            log->id = le.id;
            log->ticket_uuid = le.ticket_uuid;
            log->door_id = le.door_id;
            log->granted = le.granted;
            log->attempts = le.attempts;
            log->reason = le.reason;
            log->scanned_at = le.scanned_at;
            logList->push_back(log);
        }
        response->logs = logList;

        return createDtoResponse(Status::CODE_200, response);
    }

    // Get overall stats
    ENDPOINT("GET", "/api/stats", getStats) {

        auto& logService = service::AccessLogService::getInstance();
        auto response = dto::StatsResponseDto::createShared();

        auto overall = logService.getOverallStats();

        auto stats = dto::StatsDto::createShared();
        stats->total_tickets = overall.total_tickets;
        stats->used_tickets = overall.used_tickets;
        stats->available_tickets = overall.available_tickets;
        stats->total_access_attempts = overall.total_access_attempts;
        stats->granted_access = overall.granted_access;
        stats->denied_access = overall.denied_access;

        auto doorStatsList = oatpp::List<oatpp::Object<dto::DoorStatsDto>>::createShared();
        for (const auto& ds : overall.door_stats) {
            auto doorStats = dto::DoorStatsDto::createShared();
            doorStats->door_id = ds.door_id;
            doorStats->total_attempts = ds.total_attempts;
            doorStats->granted = ds.granted;
            doorStats->denied = ds.denied;
            doorStatsList->push_back(doorStats);
        }
        stats->door_stats = doorStatsList;

        response->success = true;
        response->stats = stats;

        return createDtoResponse(Status::CODE_200, response);
    }
};

} // namespace controller

#include OATPP_CODEGEN_END(ApiController)

#endif // ACCESS_CONTROLLER_HPP
