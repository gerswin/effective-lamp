#include "hikvision_isapi.hpp"
#include <curl/curl.h>
#include <iostream>
#include <sstream>

namespace client {

// CURL write callback
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
    size_t total_size = size * nmemb;
    output->append(static_cast<char*>(contents), total_size);
    return total_size;
}

HikvisionISAPI::HikvisionISAPI(const barcode_access::HikvisionConfig& config)
    : config_(config) {
}

HikvisionISAPI::~HikvisionISAPI() {
    if (curl_) {
        curl_easy_cleanup(static_cast<CURL*>(curl_));
        curl_ = nullptr;
    }
}

bool HikvisionISAPI::init() {
    curl_ = curl_easy_init();
    if (!curl_) {
        last_error_ = "Failed to initialize CURL";
        return false;
    }

    // Set default options
    CURL* c = static_cast<CURL*>(curl_);

    // Set authentication (Digest auth for Hikvision)
    curl_easy_setopt(c, CURLOPT_HTTPAUTH, CURLAUTH_DIGEST);
    std::string userpwd = config_.username + ":" + config_.password;
    curl_easy_setopt(c, CURLOPT_USERPWD, userpwd.c_str());

    // Set timeouts
    curl_easy_setopt(c, CURLOPT_CONNECTTIMEOUT, 5L);
    curl_easy_setopt(c, CURLOPT_TIMEOUT, 10L);

    // Disable SSL verification for internal network (optional)
    curl_easy_setopt(c, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(c, CURLOPT_SSL_VERIFYHOST, 0L);

    std::cout << "Hikvision ISAPI client initialized for " << config_.host << std::endl;
    return true;
}

std::string HikvisionISAPI::buildUrl(const std::string& endpoint) {
    std::ostringstream url;
    url << "http://" << config_.host << ":" << config_.port << endpoint;
    return url.str();
}

bool HikvisionISAPI::performRequest(const std::string& url, const std::string& method,
                                     const std::string& body, std::string* response) {
    if (!curl_) {
        last_error_ = "CURL not initialized";
        return false;
    }

    CURL* c = static_cast<CURL*>(curl_);
    std::string response_data;

    curl_easy_setopt(c, CURLOPT_URL, url.c_str());
    curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(c, CURLOPT_WRITEDATA, &response_data);

    // Set headers
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/xml");
    curl_easy_setopt(c, CURLOPT_HTTPHEADER, headers);

    // Set method
    if (method == "PUT") {
        curl_easy_setopt(c, CURLOPT_CUSTOMREQUEST, "PUT");
        if (!body.empty()) {
            curl_easy_setopt(c, CURLOPT_POSTFIELDS, body.c_str());
            curl_easy_setopt(c, CURLOPT_POSTFIELDSIZE, body.size());
        }
    } else if (method == "POST") {
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

    if (http_code >= 400) {
        last_error_ = "HTTP error: " + std::to_string(http_code);
        std::cerr << last_error_ << std::endl;
        return false;
    }

    if (response) {
        *response = response_data;
    }

    return true;
}

bool HikvisionISAPI::openDoor(int door_id) {
    // Hikvision ISAPI endpoint for door control
    // DS-K2604T uses /ISAPI/AccessControl/RemoteControl/door/{doorId}
    std::ostringstream endpoint;
    endpoint << "/ISAPI/AccessControl/RemoteControl/door/" << door_id;

    // XML body for opening door (alwaysOpen command opens momentarily)
    std::string body = R"(<?xml version="1.0" encoding="UTF-8"?>
<RemoteControlDoor>
    <cmd>open</cmd>
</RemoteControlDoor>)";

    std::string url = buildUrl(endpoint.str());
    std::cout << "Opening door " << door_id << " via ISAPI..." << std::endl;

    bool success = performRequest(url, "PUT", body);

    if (success) {
        std::cout << "Door " << door_id << " opened successfully" << std::endl;
    } else {
        std::cerr << "Failed to open door " << door_id << ": " << last_error_ << std::endl;
    }

    return success;
}

bool HikvisionISAPI::getDoorStatus(int door_id) {
    std::ostringstream endpoint;
    endpoint << "/ISAPI/AccessControl/Door/status?doorId=" << door_id;

    std::string url = buildUrl(endpoint.str());
    std::string response;

    return performRequest(url, "GET", "", &response);
}

bool HikvisionISAPI::testConnection() {
    // Test connection by getting device info
    std::string url = buildUrl("/ISAPI/System/deviceInfo");
    std::string response;

    bool success = performRequest(url, "GET", "", &response);

    if (success) {
        std::cout << "Connection to Hikvision controller successful" << std::endl;
    } else {
        std::cerr << "Failed to connect to Hikvision controller: " << last_error_ << std::endl;
    }

    return success;
}

} // namespace client
