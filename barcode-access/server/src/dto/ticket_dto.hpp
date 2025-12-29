#ifndef TICKET_DTO_HPP
#define TICKET_DTO_HPP

#include "oatpp/core/macro/codegen.hpp"
#include "oatpp/core/Types.hpp"

#include OATPP_CODEGEN_BEGIN(DTO)

namespace dto {

class TicketDto : public oatpp::DTO {
    DTO_INIT(TicketDto, DTO)

    DTO_FIELD(String, uuid);
    DTO_FIELD(Boolean, used) = false;
    DTO_FIELD(Int32, max_uses, "maxUses") = -1;
    DTO_FIELD(Int32, current_uses, "currentUses") = 0;
    DTO_FIELD(String, used_at);
    DTO_FIELD(Int32, used_at_door);
    DTO_FIELD(String, created_at);
};

class CreateTicketDto : public oatpp::DTO {
    DTO_INIT(CreateTicketDto, DTO)

    DTO_FIELD(String, uuid);
    DTO_FIELD(Int32, max_uses, "maxUses") = -1;
};

class TicketResponseDto : public oatpp::DTO {
    DTO_INIT(TicketResponseDto, DTO)

    DTO_FIELD(Boolean, success);
    DTO_FIELD(String, message);
    DTO_FIELD(Object<TicketDto>, ticket);
};

class TicketListResponseDto : public oatpp::DTO {
    DTO_INIT(TicketListResponseDto, DTO)

    DTO_FIELD(Boolean, success);
    DTO_FIELD(Int32, total);
    DTO_FIELD(Int32, used_count);
    DTO_FIELD(Int32, available_count);
    DTO_FIELD(List<Object<TicketDto>>, tickets);
};

class ValidateTicketDto : public oatpp::DTO {
    DTO_INIT(ValidateTicketDto, DTO)

    DTO_FIELD(String, uuid);
    DTO_FIELD(Int32, door_id);
};

class ValidateResponseDto : public oatpp::DTO {
    DTO_INIT(ValidateResponseDto, DTO)

    DTO_FIELD(Boolean, granted);
    DTO_FIELD(String, reason);
    DTO_FIELD(String, uuid);
    DTO_FIELD(Int32, door_id);
    DTO_FIELD(Int32, max_uses, "maxUses");
    DTO_FIELD(Int32, current_uses, "currentUses");
    DTO_FIELD(String, timestamp);
};

class RollbackTicketDto : public oatpp::DTO {
    DTO_INIT(RollbackTicketDto, DTO)

    DTO_FIELD(String, uuid);
    DTO_FIELD(Int32, door_id);
};

class RollbackResponseDto : public oatpp::DTO {
    DTO_INIT(RollbackResponseDto, DTO)

    DTO_FIELD(Boolean, success);
    DTO_FIELD(String, message);
};

} // namespace dto

#include OATPP_CODEGEN_END(DTO)

#endif // TICKET_DTO_HPP
