#ifndef CLIENT_CONFIG_DTO_HPP
#define CLIENT_CONFIG_DTO_HPP

#include "oatpp/core/macro/codegen.hpp"
#include "oatpp/core/Types.hpp"

#include OATPP_CODEGEN_BEGIN(DTO)

class ClientConfigDto : public oatpp::DTO {
    DTO_INIT(ClientConfigDto, DTO)

    DTO_FIELD(String, hardware_id);
    DTO_FIELD(Int32, door_id);
    DTO_FIELD(String, description);
    
    // Hikvision config
    DTO_FIELD(String, hik_host);
    DTO_FIELD(Int32, hik_port);
    DTO_FIELD(String, hik_user);
    DTO_FIELD(String, hik_password);
    
    // Server connection (optional, to tell client where to connect explicitly if needed)
    DTO_FIELD(String, server_url);
};

class ProvisionRequestDto : public oatpp::DTO {
    DTO_INIT(ProvisionRequestDto, DTO)

    DTO_FIELD(String, hardware_id);
    DTO_FIELD(String, current_ip); // Optional: client sends its IP
};

#include OATPP_CODEGEN_END(DTO)

#endif // CLIENT_CONFIG_DTO_HPP
