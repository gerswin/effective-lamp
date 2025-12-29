#include "oatpp/web/server/HttpConnectionHandler.hpp"
#include "oatpp/network/tcp/server/ConnectionProvider.hpp"
#include "oatpp/network/Server.hpp"
#include "oatpp/parser/json/mapping/ObjectMapper.hpp"
#include "oatpp/core/macro/component.hpp"

#include "controller/ticket_controller.hpp"
#include "controller/access_controller.hpp"
#include "controller/static_controller.hpp"
#include "db/database.hpp"
#include "common.hpp"

#include <iostream>
#include <csignal>
#include <cstdlib>
#include <fstream>

// Global server pointer for signal handling
std::atomic<bool> running{true};

void signalHandler(int signum) {
    std::cout << "\nReceived signal " << signum << ", shutting down..." << std::endl;
    running = false;
}

class AppComponent {
private:
    std::string host_;
    int port_;

public:
    AppComponent(const std::string& host, int port) : host_(host), port_(port) {}

    // Create ObjectMapper component
    OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::data::mapping::ObjectMapper>, apiObjectMapper)([] {
        return oatpp::parser::json::mapping::ObjectMapper::createShared();
    }());

    // Create ConnectionProvider component
    OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::network::ServerConnectionProvider>, serverConnectionProvider)([this] {
        return oatpp::network::tcp::server::ConnectionProvider::createShared(
            {host_, static_cast<v_uint16>(port_), oatpp::network::Address::IP_4}
        );
    }());

    // Create Router component
    OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::web::server::HttpRouter>, httpRouter)([] {
        return oatpp::web::server::HttpRouter::createShared();
    }());

    // Create ConnectionHandler component
    OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::network::ConnectionHandler>, serverConnectionHandler)([] {
        OATPP_COMPONENT(std::shared_ptr<oatpp::web::server::HttpRouter>, router);
        return oatpp::web::server::HttpConnectionHandler::createShared(router);
    }());
};

barcode_access::ServerConfig loadConfig(const std::string& configPath) {
    barcode_access::ServerConfig config;

    // Try to load from config file
    std::ifstream file(configPath);
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            // Skip comments and empty lines
            if (line.empty() || line[0] == '#') continue;

            auto pos = line.find('=');
            if (pos == std::string::npos) continue;

            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);

            // Trim whitespace
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);

            if (key == "server.host") config.bind_address = value;
            else if (key == "server.port") config.port = std::stoi(value);
            else if (key == "db.host") config.db.host = value;
            else if (key == "db.port") config.db.port = std::stoi(value);
            else if (key == "db.database") config.db.database = value;
            else if (key == "db.user") config.db.user = value;
            else if (key == "db.password") config.db.password = value;
        }
    }

    // Override with environment variables
    if (const char* env = std::getenv("SERVER_HOST")) config.bind_address = env;
    if (const char* env = std::getenv("SERVER_PORT")) config.port = std::stoi(env);
    if (const char* env = std::getenv("DB_HOST")) config.db.host = env;
    if (const char* env = std::getenv("DB_PORT")) config.db.port = std::stoi(env);
    if (const char* env = std::getenv("DB_NAME")) config.db.database = env;
    if (const char* env = std::getenv("DB_USER")) config.db.user = env;
    if (const char* env = std::getenv("DB_PASSWORD")) config.db.password = env;

    return config;
}

void run(const barcode_access::ServerConfig& config) {
    std::cout << "DEBUG: Entering run function" << std::endl;
    // Initialize Oat++ Environment
    oatpp::base::Environment::init();

    // Create application components
    AppComponent components(config.bind_address, config.port);

    // Get router and register controllers
    OATPP_COMPONENT(std::shared_ptr<oatpp::web::server::HttpRouter>, router);

    // Register API controllers
    router->addController(controller::TicketController::createShared());
    router->addController(controller::AccessController::createShared());
    router->addController(controller::StaticController::createShared("web"));

    // Get connection handler and provider
    OATPP_COMPONENT(std::shared_ptr<oatpp::network::ConnectionHandler>, connectionHandler);
    OATPP_COMPONENT(std::shared_ptr<oatpp::network::ServerConnectionProvider>, connectionProvider);

    // Create server
    oatpp::network::Server server(connectionProvider, connectionHandler);

    std::cout << "========================================" << std::endl;
    std::cout << "  Barcode Access Control Server" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Server running on http://" << config.bind_address << ":" << config.port << std::endl;
    std::cout << "API endpoints:" << std::endl;
    std::cout << "  POST /api/tickets          - Create ticket" << std::endl;
    std::cout << "  GET  /api/tickets          - List tickets" << std::endl;
    std::cout << "  GET  /api/tickets/{uuid}   - Get ticket" << std::endl;
    std::cout << "  DELETE /api/tickets/{uuid} - Delete ticket" << std::endl;
    std::cout << "  POST /api/access/validate  - Validate & use ticket" << std::endl;
    std::cout << "  GET  /api/access/logs      - Get access logs" << std::endl;
    std::cout << "  GET  /api/stats            - Get statistics" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Press Ctrl+C to stop" << std::endl;

    // Run server in a separate thread so we can check for signals
    std::thread serverThread([&server] {
        server.run();
    });

    // Wait for signal
    while (running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Stop server
    server.stop();
    connectionProvider->stop();
    serverThread.join();

    // Destroy Oat++ Environment
    oatpp::base::Environment::destroy();
}

int main(int argc, char* argv[]) {
    // Setup signal handlers
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    // Load configuration
    std::string configPath = "config.ini";
    if (argc > 1) {
        configPath = argv[1];
    }

    auto config = loadConfig(configPath);

    std::cout << "Connecting to database at " << config.db.host << ":" << config.db.port << "..." << std::endl;

    // Connect to database
    auto& db = db::Database::getInstance();
    if (!db.connect(config.db)) {
        std::cerr << "Failed to connect to database. Please check your configuration." << std::endl;
        return 1;
    }

    // Initialize schema
    if (!db.initializeSchema()) {
        std::cerr << "Failed to initialize database schema." << std::endl;
        return 1;
    }

    // Run server
    run(config);

    // Cleanup
    db.disconnect();

    std::cout << "Server stopped." << std::endl;
    return 0;
}
