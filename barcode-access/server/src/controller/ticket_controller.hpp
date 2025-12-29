#ifndef TICKET_CONTROLLER_HPP
#define TICKET_CONTROLLER_HPP

#include "oatpp/web/server/api/ApiController.hpp"
#include "oatpp/core/macro/codegen.hpp"
#include "oatpp/core/macro/component.hpp"
#include "dto/ticket_dto.hpp"
#include "service/ticket_service.hpp"

#include OATPP_CODEGEN_BEGIN(ApiController)

namespace controller {

class TicketController : public oatpp::web::server::api::ApiController {
public:
    TicketController(OATPP_COMPONENT(std::shared_ptr<ObjectMapper>, objectMapper))
        : oatpp::web::server::api::ApiController(objectMapper) {}

    static std::shared_ptr<TicketController> createShared(
        OATPP_COMPONENT(std::shared_ptr<ObjectMapper>, objectMapper)) {
        return std::make_shared<TicketController>(objectMapper);
    }

    // Create a new ticket
    ENDPOINT("POST", "/api/tickets", createTicket,
             BODY_DTO(Object<dto::CreateTicketDto>, ticketDto)) {

        auto& ticketService = service::TicketService::getInstance();
        auto response = dto::TicketResponseDto::createShared();

        if (!ticketDto->uuid || ticketDto->uuid->empty()) {
            response->success = false;
            response->message = "UUID is required";
            return createDtoResponse(Status::CODE_400, response);
        }

        std::string uuid = ticketDto->uuid;
        int max_uses = ticketDto->max_uses;

        if (!barcode_access::is_valid_uuid(uuid)) {
            response->success = false;
            response->message = "Invalid UUID format";
            return createDtoResponse(Status::CODE_400, response);
        }

        if (ticketService.createTicket(uuid, max_uses)) {
            auto ticketInfo = ticketService.getTicket(uuid);
            if (ticketInfo) {
                auto ticket = dto::TicketDto::createShared();
                ticket->uuid = ticketInfo->uuid;
                ticket->used = ticketInfo->used;
                ticket->max_uses = ticketInfo->max_uses;
                ticket->current_uses = ticketInfo->current_uses;
                ticket->used_at = ticketInfo->used_at;
                ticket->used_at_door = ticketInfo->used_at_door;
                ticket->created_at = ticketInfo->created_at;
                response->ticket = ticket;
            }
            response->success = true;
            response->message = "Ticket created successfully";
            return createDtoResponse(Status::CODE_201, response);
        } else {
            response->success = false;
            response->message = "Failed to create ticket";
            return createDtoResponse(Status::CODE_500, response);
        }
    }

    // Get ticket by UUID
    ENDPOINT("GET", "/api/tickets/{uuid}", getTicket,
             PATH(String, uuid)) {

        auto& ticketService = service::TicketService::getInstance();
        auto response = dto::TicketResponseDto::createShared();

        auto ticketInfo = ticketService.getTicket(uuid);
        if (ticketInfo) {
            auto ticket = dto::TicketDto::createShared();
            ticket->uuid = ticketInfo->uuid;
            ticket->used = ticketInfo->used;
            ticket->max_uses = ticketInfo->max_uses;
            ticket->current_uses = ticketInfo->current_uses;
            ticket->used_at = ticketInfo->used_at;
            ticket->used_at_door = ticketInfo->used_at_door;
            ticket->created_at = ticketInfo->created_at;
            response->ticket = ticket;
            response->success = true;
            response->message = "Ticket found";
            return createDtoResponse(Status::CODE_200, response);
        } else {
            response->success = false;
            response->message = "Ticket not found";
            return createDtoResponse(Status::CODE_404, response);
        }
    }

    // List all tickets
    ENDPOINT("GET", "/api/tickets", listTickets,
             QUERY(Int32, limit, "limit", 100),
             QUERY(Int32, offset, "offset", 0),
             QUERY(String, filter, "filter", "all"),
             QUERY(String, search, "search", "")) {

        auto& ticketService = service::TicketService::getInstance();
        auto response = dto::TicketListResponseDto::createShared();

        auto tickets = ticketService.getAllTickets(limit, offset, filter->c_str(), search->c_str());

        response->success = true;
        response->total = ticketService.getTotalCount();
        response->used_count = ticketService.getUsedCount();
        response->available_count = ticketService.getAvailableCount();

        auto ticketList = oatpp::List<oatpp::Object<dto::TicketDto>>::createShared();
        for (const auto& ti : tickets) {
            auto ticket = dto::TicketDto::createShared();
            ticket->uuid = ti.uuid;
            ticket->used = ti.used;
            ticket->max_uses = ti.max_uses;
            ticket->current_uses = ti.current_uses;
            ticket->used_at = ti.used_at;
            ticket->used_at_door = ti.used_at_door;
            ticket->created_at = ti.created_at;
            ticketList->push_back(ticket);
        }
        response->tickets = ticketList;

        return createDtoResponse(Status::CODE_200, response);
    }

    // Delete ticket
    ENDPOINT("DELETE", "/api/tickets/{uuid}", deleteTicket,
             PATH(String, uuid)) {

        auto& ticketService = service::TicketService::getInstance();
        auto response = dto::TicketResponseDto::createShared();

        if (ticketService.deleteTicket(uuid)) {
            response->success = true;
            response->message = "Ticket deleted successfully";
            return createDtoResponse(Status::CODE_200, response);
        } else {
            response->success = false;
            response->message = "Failed to delete ticket";
            return createDtoResponse(Status::CODE_500, response);
        }
    }
};

} // namespace controller

#include OATPP_CODEGEN_END(ApiController)

#endif // TICKET_CONTROLLER_HPP
