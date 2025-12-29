#include <curl/curl.h>
#include <poll.h>
#include "barcode_reader.hpp"
#include "hikvision_isapi.hpp"
#include "db_client.hpp"
#include "discovery_client.hpp"
#include "common.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <csignal>
#include <cstdlib>
#include <thread>
#include <chrono>
#include <random>
#include <iomanip>

// Global flag for signal handling
std::atomic<bool> running{true};

void signalHandler(int signum) {
    std::cout << "\nReceived signal " << signum << ", shutting down..." << std::endl;
    running = false;
}

struct ClientConfig {
    int door_id = 0; // 0 means unconfigured
    std::string input_device = "/dev/input/event0";
    std::string server_url = "auto";
    barcode_access::HikvisionConfig hikvision;
};

// Generate a random hex string
std::string generateRandomHex(int length) {
    static const char hex_chars[] = "0123456789abcdef";
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    
    std::string s;
    for (int i = 0; i < length; ++i) {
        s += hex_chars[dis(gen)];
    }
    return s;
}

std::string getOrCreateHardwareId() {
    const std::string filename = ".device_id";
    std::ifstream file(filename);
    std::string id;
    if (file.is_open() && std::getline(file, id) && !id.empty()) {
        return id;
    }
    
    // Generate new ID
    id = generateRandomHex(16);
    std::ofstream outfile(filename);
    outfile << id;
    return id;
}

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

void saveConfigToFile(const std::string& configPath, const ClientConfig& config) {
    std::ofstream file(configPath);
    if (!file.is_open()) return;
    
    file << "# Auto-generated config\n";
    file << "door.id=" << config.door_id << "\n";
    file << "input.device=" << config.input_device << "\n";
    file << "server.url=" << config.server_url << "\n";
    file << "\n";
    file << "# Hikvision Controller\n";
    file << "hikvision.host=" << config.hikvision.host << "\n";
    file << "hikvision.port=" << config.hikvision.port << "\n";
    file << "hikvision.username=" << config.hikvision.username << "\n";
    file << "hikvision.password=" << config.hikvision.password << "\n";
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
}

int main(int argc, char* argv[]) {
    // Setup signal handlers
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    // Initialize CURL globally
    curl_global_init(CURL_GLOBAL_DEFAULT);

    std::string configPath = "client.ini";
    if (argc > 1) {
        configPath = argv[1];
    }

    ClientConfig config = loadConfig(configPath);
    std::string hardwareId = getOrCreateHardwareId();
    std::cout << "Hardware ID: " << hardwareId << std::endl;

    // 1. Auto-Discovery
    if (config.server_url == "auto" || config.server_url.empty()) {
        std::cout << "Searching for server (UDP Broadcast)..." << std::endl;
        client::DiscoveryClient discovery;
        while (running && (config.server_url == "auto" || config.server_url.empty())) {
            std::string url = discovery.findServer(3000); // 3s timeout
            if (!url.empty()) {
                config.server_url = url;
                std::cout << "Found server at: " << url << std::endl;
            } else {
                std::cout << "Server not found. Retrying..." << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
    }
    
    if (!running) return 0;

    // Initialize access client
    client::AccessClient accessClient(config.server_url);
    if (!accessClient.init()) {
        std::cerr << "Failed to initialize access client" << std::endl;
        return 1;
    }

    // 2. Auto-Provisioning
    // If we have no Door ID assigned, we ask the server "Who am I?"
    if (config.door_id == 0) {
        std::cout << "Door ID not configured. Requesting auto-provisioning..." << std::endl;
        
        while (running && config.door_id == 0) {
            auto provisionConfig = accessClient.provision(hardwareId);
            if (provisionConfig.success) {
                std::cout << "Provisioning successful!" << std::endl;
                std::cout << "Assigned Door ID: " << provisionConfig.door_id << std::endl;
                
                config.door_id = provisionConfig.door_id;
                config.hikvision.host = provisionConfig.hik_host;
                config.hikvision.port = provisionConfig.hik_port;
                config.hikvision.username = provisionConfig.hik_user;
                config.hikvision.password = provisionConfig.hik_password;
                
                // Save updated config
                saveConfigToFile(configPath, config);
                std::cout << "Configuration saved to " << configPath << std::endl;
            } else {
                std::cerr << "Provisioning failed. Retrying in 5s..." << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(5));
            }
        }
    }

    printBanner(config);

    // Test connection to central server
    if (!accessClient.testConnection()) {
        std::cerr << "Error: Cannot connect to central server." << std::endl;
        return 1;
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
        if (!barcode_access::is_valid_ticket_code(barcode)) {
            std::cout << "Result: DENIED (Invalid Ticket Code format)" << std::endl;
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
                
                // Attempt to rollback the ticket usage
                std::cout << "Attempting to rollback ticket usage..." << std::endl;
                if (accessClient.rollbackUsage(barcode, config.door_id)) {
                    std::cout << "Ticket usage rolled back successfully." << std::endl;
                } else {
                    std::cout << "Failed to rollback ticket usage. Please contact support." << std::endl;
                }
            }
        }

        std::cout << "----------------------------------------" << std::endl;
    };

    // Initialize barcode reader
    client::BarcodeReader reader(config.input_device);
    if (!reader.start(onBarcodeScanned)) {
        std::cerr << "Failed to start barcode reader: " << reader.getLastError() << std::endl;
        std::cout << "Falling back to keyboard input for testing." << std::endl;
        
        // Polling loop for keyboard input
        struct pollfd fds;
        fds.fd = STDIN_FILENO;
        fds.events = POLLIN;

        std::string input;
        while (running) {
            int ret = poll(&fds, 1, 100); 
            if (ret > 0) {
                if (std::getline(std::cin, input) && !input.empty()) {
                    onBarcodeScanned(input);
                }
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