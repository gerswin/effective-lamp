#ifndef STATIC_CONTROLLER_HPP
#define STATIC_CONTROLLER_HPP

#include "oatpp/web/server/api/ApiController.hpp"
#include "oatpp/core/macro/codegen.hpp"
#include "oatpp/core/macro/component.hpp"
#include <fstream>
#include <sstream>

#include OATPP_CODEGEN_BEGIN(ApiController)

namespace controller {

class StaticController : public oatpp::web::server::api::ApiController {
private:
    std::string webRoot_;

    std::string readFile(const std::string& path) {
        std::ifstream file(path);
        if (!file.is_open()) {
            return "";
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }

    std::string getContentType(const std::string& path) {
        if (path.ends_with(".html")) return "text/html";
        if (path.ends_with(".css")) return "text/css";
        if (path.ends_with(".js")) return "application/javascript";
        if (path.ends_with(".json")) return "application/json";
        if (path.ends_with(".png")) return "image/png";
        if (path.ends_with(".jpg") || path.ends_with(".jpeg")) return "image/jpeg";
        if (path.ends_with(".svg")) return "image/svg+xml";
        if (path.ends_with(".ico")) return "image/x-icon";
        return "text/plain";
    }

public:
    StaticController(const std::string& webRoot,
                     OATPP_COMPONENT(std::shared_ptr<ObjectMapper>, objectMapper))
        : oatpp::web::server::api::ApiController(objectMapper)
        , webRoot_(webRoot) {}

    static std::shared_ptr<StaticController> createShared(
        const std::string& webRoot,
        OATPP_COMPONENT(std::shared_ptr<ObjectMapper>, objectMapper)) {
        return std::make_shared<StaticController>(webRoot, objectMapper);
    }

    // Serve index.html at root
    ENDPOINT("GET", "/", index) {
        auto content = readFile(webRoot_ + "/index.html");
        if (content.empty()) {
            return createResponse(Status::CODE_404, "Not Found");
        }
        auto response = createResponse(Status::CODE_200, content);
        response->putHeader("Content-Type", "text/html; charset=utf-8");
        return response;
    }

    // Serve static files
    ENDPOINT("GET", "/static/{path}", staticFile,
             PATH(String, path)) {
        std::string filePath = webRoot_ + "/static/" + path->c_str();
        auto content = readFile(filePath);
        if (content.empty()) {
            return createResponse(Status::CODE_404, "Not Found");
        }
        auto response = createResponse(Status::CODE_200, content);
        response->putHeader("Content-Type", getContentType(filePath));
        return response;
    }
};

} // namespace controller

#include OATPP_CODEGEN_END(ApiController)

#endif // STATIC_CONTROLLER_HPP
