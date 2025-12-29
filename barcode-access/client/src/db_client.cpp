#include "db_client.hpp"
#include <curl/curl.h>
#include <iostream>
#include <sstream>

namespace client {

// Simple JSON parsing helpers (minimal, no external dependency)
static std::string extractJsonString(const std::string& json, const std::string& key) {
    std::string search = "\"" + key + "\":";
    auto pos = json.find(search);
    if (pos == std::string::npos) return "";

    pos += search.length();

    // Skip whitespace
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) pos++;

    if (pos >= json.size()) return "";

    if (json[pos] == '"') {
        // String value
        pos++;
        auto end = json.find('"', pos);
        if (end == std::string::npos) return "";
        return json.substr(pos, end - pos);
    } else {
        // Non-string value (number, boolean)
        auto end = json.find_first_of(",}", pos);
        if (end == std::string::npos) return "";
        return json.substr(pos, end - pos);
    }
}

static bool extractJsonBool(const std::string& json, const std::string& key) {
    std::string value = extractJsonString(json, key);
    return value == "true";
}

static int extractJsonInt(const std::string& json, const std::string& key) {
    std::string value = extractJsonString(json, key);
    try {
        return std::stoi(value);
    } catch (...) {
        return 0;
    }
}

// CURL write callback
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
    size_t total_size = size * nmemb;
    output->append(static_cast<char*>(contents), total_size);
    return total_size;
}

AccessClient::AccessClient(const std::string& server_url)
    : server_url_(server_url) {
    // Remove trailing slash if present
    if (!server_url_.empty() && server_url_.back() == '/') {
        server_url_.pop_back();
    }
}

AccessClient::~AccessClient() {
    if (curl_) {
        curl_easy_cleanup(static_cast<CURL*>(curl_));
        curl_ = nullptr;
    }
}

bool AccessClient::init() {
    curl_ = curl_easy_init();
    if (!curl_) {
        last_error_ = "Failed to initialize CURL";
        return false;
    }

    CURL* c = static_cast<CURL*>(curl_);

    // Set timeouts
    curl_easy_setopt(c, CURLOPT_CONNECTTIMEOUT, 5L);
    curl_easy_setopt(c, CURLOPT_TIMEOUT, 10L);

    std::cout << "Access client initialized for server: " << server_url_ << std::endl;
    return true;
}

bool AccessClient::performRequest(const std::string& url, const std::string& method,
                                   const std::string& body, std::string* response) {
    if (!curl_) {
        last_error_ = "CURL not initialized";
        return false;
    }

    CURL* c = static_cast<CURL*>(curl_);
    std::string response_data;

    // Reset CURL options for new request
    curl_easy_reset(c);

    curl_easy_setopt(c, CURLOPT_URL, url.c_str());
    curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(c, CURLOPT_WRITEDATA, &response_data);
    curl_easy_setopt(c, CURLOPT_CONNECTTIMEOUT, 5L);
    curl_easy_setopt(c, CURLOPT_TIMEOUT, 10L);

    // Set headers
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "Accept: application/json");
    curl_easy_setopt(c, CURLOPT_HTTPHEADER, headers);

    // Set method
    if (method == "POST") {
        curl_easy_setopt(c, CURLOPT_POST, 1L);
        if (!body.empty()) {
            curl_easy_setopt(c, CURLOPT_POSTFIELDS, body.c_str());
            curl_easy_setopt(c, CURLOPT_POSTFIELDSIZE, body.size());
        }
    } else {
        curl_easy_setopt(c, CURLOPT_HTTPGET, 1L);
    }

    // Perform request
    CURLcode res = curl_easy_perform(c);

    // Cleanup headers
    curl_slist_free_all(headers);

    if (res != CURLE_OK) {
        last_error_ = "CURL error: " + std::string(curl_easy_strerror(res));
        std::cerr << last_error_ << std::endl;
        return false;
    }

    long http_code = 0;
    curl_easy_getinfo(c, CURLINFO_RESPONSE_CODE, &http_code);

    if (http_code >= 500) {
        last_error_ = "Server error: " + std::to_string(http_code);
        std::cerr << last_error_ << std::endl;
        return false;
    }

    if (response) {
        *response = response_data;
    }

    return true;
}

ValidationResponse AccessClient::validateTicket(const std::string& uuid, int door_id) {
    ValidationResponse vr;
    vr.uuid = uuid;
    vr.door_id = door_id;
    vr.granted = false;
    vr.reason = "ERROR";

    // Build JSON request
    std::ostringstream json;
    json << "{\"uuid\":\"" << uuid << "\",\"door_id\":" << door_id << "}";

    std::string url = server_url_ + "/api/access/validate";
    std::string response;

    if (!performRequest(url, "POST", json.str(), &response)) {
        vr.reason = "CONNECTION_ERROR: " + last_error_;
        return vr;
    }

    // Parse response
    vr.granted = extractJsonBool(response, "granted");
    vr.reason = extractJsonString(response, "reason");

    return vr;
}

bool AccessClient::rollbackUsage(const std::string& uuid, int door_id) {
    // Build JSON request
    std::ostringstream json;
    json << "{\"uuid\":\"" << uuid << "\",\"door_id\":" << door_id << "}";

    std::string url = server_url_ + "/api/access/rollback";
    std::string response;

    if (!performRequest(url, "POST", json.str(), &response)) {
        return false;
    }

    // Parse response
    return extractJsonBool(response, "success");
}

bool AccessClient::testConnection() {
    std::string url = server_url_ + "/api/stats";
    std::string response;

    bool success = performRequest(url, "GET", "", &response);

    if (success) {
        std::cout << "Connection to central server successful" << std::endl;
    } else {
        std::cerr << "Failed to connect to central server: " << last_error_ << std::endl;
    }

    return success;
}

} // namespace client
