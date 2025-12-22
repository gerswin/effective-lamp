#ifndef ACCESS_LOG_DTO_HPP
#define ACCESS_LOG_DTO_HPP

#include "oatpp/core/macro/codegen.hpp"
#include "oatpp/core/Types.hpp"

#include OATPP_CODEGEN_BEGIN(DTO)

namespace dto {

class AccessLogDto : public oatpp::DTO {
    DTO_INIT(AccessLogDto, DTO)

    DTO_FIELD(Int64, id);
    DTO_FIELD(String, ticket_uuid);
    DTO_FIELD(Int32, door_id);
    DTO_FIELD(Boolean, granted);
    DTO_FIELD(Int32, attempts);
    DTO_FIELD(String, reason);
    DTO_FIELD(String, scanned_at);
};

class AccessLogListResponseDto : public oatpp::DTO {
    DTO_INIT(AccessLogListResponseDto, DTO)

    DTO_FIELD(Boolean, success);
    DTO_FIELD(Int32, total);
    DTO_FIELD(Int32, granted_count);
    DTO_FIELD(Int32, denied_count);
    DTO_FIELD(List<Object<AccessLogDto>>, logs);
};

class DoorStatsDto : public oatpp::DTO {
    DTO_INIT(DoorStatsDto, DTO)

    DTO_FIELD(Int32, door_id);
    DTO_FIELD(Int32, total_attempts);
    DTO_FIELD(Int32, granted);
    DTO_FIELD(Int32, denied);
};

class StatsDto : public oatpp::DTO {
    DTO_INIT(StatsDto, DTO)

    DTO_FIELD(Int32, total_tickets);
    DTO_FIELD(Int32, used_tickets);
    DTO_FIELD(Int32, available_tickets);
    DTO_FIELD(Int32, total_access_attempts);
    DTO_FIELD(Int32, granted_access);
    DTO_FIELD(Int32, denied_access);
    DTO_FIELD(List<Object<DoorStatsDto>>, door_stats);
};

class StatsResponseDto : public oatpp::DTO {
    DTO_INIT(StatsResponseDto, DTO)

    DTO_FIELD(Boolean, success);
    DTO_FIELD(Object<StatsDto>, stats);
};

} // namespace dto

#include OATPP_CODEGEN_END(DTO)

#endif // ACCESS_LOG_DTO_HPP
