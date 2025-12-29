#ifndef PROVISION_CONTROLLER_HPP
#define PROVISION_CONTROLLER_HPP

#include "oatpp/web/server/api/ApiController.hpp"
#include "oatpp/core/macro/codegen.hpp"
#include "oatpp/core/macro/component.hpp"

#include "dto/client_config_dto.hpp"
#include "db/database.hpp"
#include <iostream>

namespace controller {

#include OATPP_CODEGEN_BEGIN(ApiController)

class ProvisionController : public oatpp::web::server::api::ApiController {
public:
    ProvisionController(const std::shared_ptr<ObjectMapper>& objectMapper)
        : oatpp::web::server::api::ApiController(objectMapper)
    {}

    static std::shared_ptr<ProvisionController> createShared(
        OATPP_COMPONENT(std::shared_ptr<ObjectMapper>, objectMapper)
    ) {
        return std::make_shared<ProvisionController>(objectMapper);
    }

    ENDPOINT("POST", "/api/provision", provision, BODY_DTO(Object<ProvisionRequestDto>, requestDto)) {
        if (!requestDto->hardware_id) {
            return createResponse(Status::CODE_400, "Missing hardware_id");
        }

        std::string hardwareId = requestDto->hardware_id;
        auto& db = db::Database::getInstance();
        
        // 1. Check if client exists
        {
            std::string query = "SELECT door_id, hik_host, hik_port, hik_user, hik_password FROM client_configs WHERE hardware_id = $1";
            std::vector<std::string> params = { hardwareId };
            db::PGResultGuard result(db.executeParams(query, params));

            if (result.ok() && result.rowCount() > 0) {
                auto responseDto = ClientConfigDto::createShared();
                responseDto->hardware_id = hardwareId;
                responseDto->door_id = std::stoi(result.getValue(0, 0));
                responseDto->hik_host = result.getValue(0, 1);
                
                std::string portStr = result.getValue(0, 2);
                responseDto->hik_port = portStr.empty() ? 80 : std::stoi(portStr);
                
                responseDto->hik_user = result.getValue(0, 3);
                responseDto->hik_password = result.getValue(0, 4);
                
                return createDtoResponse(Status::CODE_200, responseDto);
            }
        }

        // 2. Register new client
        // Find next door_id
        int nextDoorId = 1;
        {
            std::string query = "SELECT MAX(door_id) FROM client_configs";
            db::PGResultGuard result(db.execute(query));
            if (result.ok() && result.rowCount() > 0) {
                std::string val = result.getValue(0, 0);
                if (!val.empty()) {
                    nextDoorId = std::stoi(val) + 1;
                }
            }
        }

        // Insert default config
        {
            std::string query = "INSERT INTO client_configs (hardware_id, door_id, hik_host, hik_port, hik_user, hik_password) VALUES ($1, $2, $3, $4, $5, $6)";
            std::vector<std::string> params = {
                hardwareId,
                std::to_string(nextDoorId),
                "192.168.1.64", // Default Hikvision IP placeholder
                "80",
                "admin",
                "password"
            };
            
            db::PGResultGuard result(db.executeParams(query, params));
            if (!result.ok()) {
                 return createResponse(Status::CODE_500, "Database error creating client config");
            }
        }

        // Return new config
        auto responseDto = ClientConfigDto::createShared();
        responseDto->hardware_id = hardwareId;
        responseDto->door_id = nextDoorId;
        responseDto->hik_host = "192.168.1.64";
        responseDto->hik_port = 80;
        responseDto->hik_user = "admin";
        responseDto->hik_password = "password";

        return createDtoResponse(Status::CODE_200, responseDto);
    }
};

#include OATPP_CODEGEN_END(ApiController)

} // namespace controller

#endif // PROVISION_CONTROLLER_HPP
