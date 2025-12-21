#include "barcode_reader.hpp"
#include "hikvision_isapi.hpp"
#include "db_client.hpp"
#include "common.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <csignal>
#include <cstdlib>
#include <thread>
#include <chrono>

// Global flag for signal handling
std::atomic<bool> running{true};

void signalHandler(int signum) {
    std::cout << "\nReceived signal " << signum << ", shutting down..." << std::endl;
    running = false;
}

struct ClientConfig {
    int door_id = 1;
    std::string input_device = "/dev/input/event0";
    std::string server_url = "http://localhost:8080";
    barcode_access::HikvisionConfig hikvision;
};

ClientConfig loadConfig(const std::string& configPath) {
    ClientConfig config;

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

            if (key == "door.id") config.door_id = std::stoi(value);
            else if (key == "input.device") config.input_device = value;
            else if (key == "server.url") config.server_url = value;
            else if (key == "hikvision.host") config.hikvision.host = value;
            else if (key == "hikvision.port") config.hikvision.port = std::stoi(value);
            else if (key == "hikvision.username") config.hikvision.username = value;
            else if (key == "hikvision.password") config.hikvision.password = value;
        }
    }

    // Override with environment variables
    if (const char* env = std::getenv("DOOR_ID")) config.door_id = std::stoi(env);
    if (const char* env = std::getenv("INPUT_DEVICE")) config.input_device = env;
    if (const char* env = std::getenv("SERVER_URL")) config.server_url = env;
    if (const char* env = std::getenv("HIK_HOST")) config.hikvision.host = env;
    if (const char* env = std::getenv("HIK_PORT")) config.hikvision.port = std::stoi(env);
    if (const char* env = std::getenv("HIK_USER")) config.hikvision.username = env;
    if (const char* env = std::getenv("HIK_PASSWORD")) config.hikvision.password = env;

    return config;
}

void printBanner(const ClientConfig& config) {
    std::cout << "========================================" << std::endl;
    std::cout << "  Barcode Access Control Client" << std::endl;
    std::cout << "  Door #" << config.door_id << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Input device: " << config.input_device << std::endl;
    std::cout << "Server URL: " << config.server_url << std::endl;
    std::cout << "Hikvision: " << config.hikvision.host << ":" << config.hikvision.port << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Waiting for barcode scans..." << std::endl;
    std::cout << "Press Ctrl+C to stop" << std::endl;
    std::cout << std::endl;
}

int main(int argc, char* argv[]) {
    // Setup signal handlers
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    // Load configuration
    std::string configPath = "client.ini";
    if (argc > 1) {
        configPath = argv[1];
    }

    ClientConfig config = loadConfig(configPath);
    printBanner(config);

    // Initialize CURL globally
    curl_global_init(CURL_GLOBAL_DEFAULT);

    // Initialize access client (connects to central server)
    client::AccessClient accessClient(config.server_url);
    if (!accessClient.init()) {
        std::cerr << "Failed to initialize access client" << std::endl;
        return 1;
    }

    // Test connection to central server
    if (!accessClient.testConnection()) {
        std::cerr << "Warning: Cannot connect to central server. Continuing anyway..." << std::endl;
    }

    // Initialize Hikvision ISAPI client
    client::HikvisionISAPI hikvision(config.hikvision);
    if (!hikvision.init()) {
        std::cerr << "Failed to initialize Hikvision client" << std::endl;
        return 1;
    }

    // Test connection to Hikvision controller
    if (!hikvision.testConnection()) {
        std::cerr << "Warning: Cannot connect to Hikvision controller. Continuing anyway..." << std::endl;
    }

    // Barcode scan callback
    auto onBarcodeScanned = [&](const std::string& barcode) {
        std::cout << "----------------------------------------" << std::endl;
        std::cout << "Scanned: " << barcode << std::endl;

        // Validate UUID format
        if (!barcode_access::is_valid_uuid(barcode)) {
            std::cout << "Result: DENIED (Invalid UUID format)" << std::endl;
            std::cout << "----------------------------------------" << std::endl;
            return;
        }

        // Validate ticket with central server
        auto result = accessClient.validateTicket(barcode, config.door_id);

        std::cout << "Result: " << (result.granted ? "GRANTED" : "DENIED") << std::endl;
        std::cout << "Reason: " << result.reason << std::endl;

        if (result.granted) {
            // Open the door
            if (hikvision.openDoor(config.door_id)) {
                std::cout << "Door " << config.door_id << " OPENED" << std::endl;
            } else {
                std::cout << "Warning: Failed to open door: " << hikvision.getLastError() << std::endl;
            }
        }

        std::cout << "----------------------------------------" << std::endl;
    };

    // Initialize barcode reader
    client::BarcodeReader reader(config.input_device);
    if (!reader.start(onBarcodeScanned)) {
        std::cerr << "Failed to start barcode reader: " << reader.getLastError() << std::endl;
        std::cerr << "Make sure the input device exists and you have permission to read it." << std::endl;
        std::cerr << "Try running with sudo or add your user to the 'input' group." << std::endl;

        // Fall back to stdin for testing
        std::cout << "\nFalling back to keyboard input for testing." << std::endl;
        std::cout << "Type UUID and press Enter to simulate scan:" << std::endl;

        std::string input;
        while (running && std::getline(std::cin, input)) {
            if (!input.empty()) {
                onBarcodeScanned(input);
            }
        }
    } else {
        // Main loop - wait for signal
        while (running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        reader.stop();
    }

    // Cleanup
    curl_global_cleanup();

    std::cout << "Client stopped." << std::endl;
    return 0;
}
