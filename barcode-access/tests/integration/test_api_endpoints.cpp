#include <gtest/gtest.h>
#include "mock_database.hpp"
#include "common.hpp"
#include <string>

using namespace mocks;

// Integration tests for API endpoint logic
// These test the complete flow without actual HTTP

class APIEndpointTest : public ::testing::Test {
protected:
    MockDatabase* db;

    void SetUp() override {
        db = &MockDatabase::getInstance();
        db->reset();
    }

    void TearDown() override {
        db->reset();
    }

    // Simulate API responses
    struct APIResponse {
        int status_code;
        bool success;
        std::string message;
    };

    // Simulate POST /api/tickets
    APIResponse createTicketEndpoint(const std::string& uuid, int max_uses = -1) {
        APIResponse response;

        if (uuid.empty()) {
            response.status_code = 400;
            response.success = false;
            response.message = "UUID is required";
            return response;
        }

        if (!barcode_access::is_valid_uuid(uuid)) {
            response.status_code = 400;
            response.success = false;
            response.message = "Invalid UUID format";
            return response;
        }

        if (db->createTicket(uuid, max_uses)) {
            response.status_code = 201;
            response.success = true;
            response.message = "Ticket created successfully";
        } else {
            response.status_code = 500;
            response.success = false;
            response.message = "Failed to create ticket";
        }

        return response;
    }

    // Simulate GET /api/tickets/{uuid}
    APIResponse getTicketEndpoint(const std::string& uuid) {
        APIResponse response;

        auto ticket = db->getTicket(uuid);
        if (ticket.has_value()) {
            response.status_code = 200;
            response.success = true;
            response.message = "Ticket found";
        } else {
            response.status_code = 404;
            response.success = false;
            response.message = "Ticket not found";
        }

        return response;
    }

    // Simulate DELETE /api/tickets/{uuid}
    APIResponse deleteTicketEndpoint(const std::string& uuid) {
        APIResponse response;

        if (db->deleteTicket(uuid)) {
            response.status_code = 200;
            response.success = true;
            response.message = "Ticket deleted successfully";
        } else {
            response.status_code = 500;
            response.success = false;
            response.message = "Failed to delete ticket";
        }

        return response;
    }

    // Simulate validation result
    struct ValidationResult {
        bool granted;
        std::string reason;
        std::string uuid;
        int door_id;
        int max_uses;
        int current_uses;
    };

    // Simulate POST /api/access/validate
    ValidationResult validateAccessEndpoint(const std::string& uuid, int door_id) {
        ValidationResult result;
        result.uuid = uuid;
        result.door_id = door_id;
        result.max_uses = -1; // Default
        result.current_uses = 0; // Default

        // Check UUID format
        if (!barcode_access::is_valid_uuid(uuid)) {
            result.granted = false;
            result.reason = barcode_access::access_result_to_string(barcode_access::AccessResult::DENIED_INVALID_UUID);
            db->logAccess(uuid, door_id, false, result.reason);
            return result;
        }

        // Check if ticket exists
        auto ticket_opt = db->getTicket(uuid);
        if (!ticket_opt.has_value()) {
            result.granted = false;
            result.reason = barcode_access::access_result_to_string(barcode_access::AccessResult::DENIED_NOT_FOUND);
            db->logAccess(uuid, door_id, false, result.reason);
            return result;
        }

        MockTicket ticket = ticket_opt.value();
        result.max_uses = ticket.max_uses;
        result.current_uses = ticket.current_uses;

        // Check against max_uses first for limited tickets
        if (ticket.max_uses != -1 && ticket.current_uses >= ticket.max_uses) {
            result.granted = false;
            result.reason = barcode_access::access_result_to_string(barcode_access::AccessResult::DENIED_MAX_USES_REACHED);
            db->logAccess(uuid, door_id, false, result.reason);
            return result;
        }

        // Check if already used (for single-use tickets, or for unlimited tickets if set explicitly)
        if (ticket.used && (ticket.max_uses == -1 || ticket.max_uses == 1 && ticket.current_uses == 1)) {
            result.granted = false;
            result.reason = barcode_access::access_result_to_string(barcode_access::AccessResult::DENIED_ALREADY_USED);
            db->logAccess(uuid, door_id, false, result.reason);
            return result;
        }

        // Mark as used and grant access
        db->markTicketUsed(uuid, door_id);
        result.granted = true;
        result.reason = barcode_access::access_result_to_string(barcode_access::AccessResult::GRANTED);
        db->logAccess(uuid, door_id, true, result.reason);
        
        // Update current_uses for the result after markTicketUsed
        ticket_opt = db->getTicket(uuid);
        if(ticket_opt.has_value()){
            result.current_uses = ticket_opt->current_uses;
            result.max_uses = ticket_opt->max_uses;
        }

        return result;
    }
};

// Test ticket creation endpoint
TEST_F(APIEndpointTest, CreateTicket_Success) {
    auto response = createTicketEndpoint("550e8400-e29b-41d4-a716-446655440000", -1);

    EXPECT_EQ(response.status_code, 201);
    EXPECT_TRUE(response.success);
    EXPECT_EQ(response.message, "Ticket created successfully");
    EXPECT_EQ(db->getTotalTickets(), 1);
}

TEST_F(APIEndpointTest, CreateTicket_EmptyUUID) {
    auto response = createTicketEndpoint("");

    EXPECT_EQ(response.status_code, 400);
    EXPECT_FALSE(response.success);
    EXPECT_EQ(response.message, "UUID is required");
}

TEST_F(APIEndpointTest, CreateTicket_InvalidUUID) {
    auto response = createTicketEndpoint("not-a-valid-uuid");

    EXPECT_EQ(response.status_code, 400);
    EXPECT_FALSE(response.success);
    EXPECT_EQ(response.message, "Invalid UUID format");
}

// Test get ticket endpoint
TEST_F(APIEndpointTest, GetTicket_Found) {
    db->createTicket("550e8400-e29b-41d4-a716-446655440000", -1);
    auto response = getTicketEndpoint("550e8400-e29b-41d4-a716-446655440000");

    EXPECT_EQ(response.status_code, 200);
    EXPECT_TRUE(response.success);
}

TEST_F(APIEndpointTest, GetTicket_NotFound) {
    auto response = getTicketEndpoint("550e8400-e29b-41d4-a716-446655440000");

    EXPECT_EQ(response.status_code, 404);
    EXPECT_FALSE(response.success);
}

// Test delete ticket endpoint
TEST_F(APIEndpointTest, DeleteTicket_Success) {
    db->createTicket("550e8400-e29b-41d4-a716-446655440000", -1);
    auto response = deleteTicketEndpoint("550e8400-e29b-41d4-a716-446655440000");

    EXPECT_EQ(response.status_code, 200);
    EXPECT_TRUE(response.success);
    EXPECT_EQ(db->getTotalTickets(), 0);
}

TEST_F(APIEndpointTest, DeleteTicket_NotExists) {
    auto response = deleteTicketEndpoint("550e8400-e29b-41d4-a716-446655440000");

    EXPECT_EQ(response.status_code, 500);
    EXPECT_FALSE(response.success);
}

// Test access validation endpoint
TEST_F(APIEndpointTest, ValidateAccess_Granted) {
    std::string uuid = "550e8400-e29b-41d4-a716-446655440000";
    db->createTicket(uuid, -1);

    auto result = validateAccessEndpoint(uuid, 1);

    EXPECT_TRUE(result.granted);
    EXPECT_EQ(result.reason, barcode_access::access_result_to_string(barcode_access::AccessResult::GRANTED));
    EXPECT_EQ(result.door_id, 1);
    EXPECT_EQ(result.current_uses, 1);
    EXPECT_EQ(result.max_uses, -1);

    // Verify ticket state in DB
    auto ticket_db = db->getTicket(uuid);
    ASSERT_TRUE(ticket_db.has_value());
    EXPECT_TRUE(ticket_db->used); // Mark as used in mock
    EXPECT_EQ(ticket_db->current_uses, 1);
}

TEST_F(APIEndpointTest, ValidateAccess_DeniedNotFound) {
    auto result = validateAccessEndpoint("550e8400-e29b-41d4-a716-446655440000", 1);

    EXPECT_FALSE(result.granted);
    EXPECT_EQ(result.reason, barcode_access::access_result_to_string(barcode_access::AccessResult::DENIED_NOT_FOUND));
}

TEST_F(APIEndpointTest, ValidateAccess_DeniedAlreadyUsed) {
    std::string uuid = "550e8400-e29b-41d4-a716-446655440000";
    db->createTicket(uuid, 1); // 1 use
    db->markTicketUsed(uuid, 1); // Use it once, now it's fully used

    auto result = validateAccessEndpoint(uuid, 2);

    EXPECT_FALSE(result.granted);
    // The reason should now be DENIED_MAX_USES_REACHED if the service logic is correctly implemented
    EXPECT_EQ(result.reason, barcode_access::access_result_to_string(barcode_access::AccessResult::DENIED_MAX_USES_REACHED));
}

TEST_F(APIEndpointTest, ValidateAccess_DeniedInvalidUUID) {
    auto result = validateAccessEndpoint("invalid-uuid", 1);

    EXPECT_FALSE(result.granted);
    EXPECT_EQ(result.reason, barcode_access::access_result_to_string(barcode_access::AccessResult::DENIED_INVALID_UUID));
}

// Test access logging
TEST_F(APIEndpointTest, ValidateAccess_LogsGranted) {
    std::string uuid = "550e8400-e29b-41d4-a716-446655440000";
    db->createTicket(uuid, -1);

    validateAccessEndpoint(uuid, 1);

    auto logs = db->getLogsByTicket(uuid);
    ASSERT_EQ(logs.size(), 1);
    EXPECT_TRUE(logs[0].granted);
    EXPECT_EQ(logs[0].reason, barcode_access::access_result_to_string(barcode_access::AccessResult::GRANTED));
}

TEST_F(APIEndpointTest, ValidateAccess_LogsDenied) {
    std::string uuid = "550e8400-e29b-41d4-a716-446655440000";
    auto result = validateAccessEndpoint(uuid, 1); // Try to validate a non-existent ticket

    EXPECT_EQ(db->getDeniedCount(), 1);
    EXPECT_EQ(result.reason, barcode_access::access_result_to_string(barcode_access::AccessResult::DENIED_NOT_FOUND));
}

// Test multiple door access
TEST_F(APIEndpointTest, ValidateAccess_MultipleDoors) {
    std::string uuid1 = "550e8400-e29b-41d4-a716-446655440001";
    std::string uuid2 = "550e8400-e29b-41d4-a716-446655440002";
    std::string uuid3 = "550e8400-e29b-41d4-a716-446655440003";
    std::string uuid4 = "550e8400-e29b-41d4-a716-446655440004";

    db->createTicket(uuid1, -1);
    db->createTicket(uuid2, -1);
    db->createTicket(uuid3, -1);
    db->createTicket(uuid4, -1);

    EXPECT_TRUE(validateAccessEndpoint(uuid1, 1).granted);
    EXPECT_TRUE(validateAccessEndpoint(uuid2, 2).granted);
    EXPECT_TRUE(validateAccessEndpoint(uuid3, 3).granted);
    EXPECT_TRUE(validateAccessEndpoint(uuid4, 4).granted);

    // Each door should have one log
    EXPECT_EQ(db->getLogsByDoor(1).size(), 1);
    EXPECT_EQ(db->getLogsByDoor(2).size(), 1);
    EXPECT_EQ(db->getLogsByDoor(3).size(), 1);
    EXPECT_EQ(db->getLogsByDoor(4).size(), 1);
}

// Test same ticket on different doors
TEST_F(APIEndpointTest, ValidateAccess_SameTicketDifferentDoors) {
    std::string uuid = "550e8400-e29b-41d4-a716-446655440000";
    db->createTicket(uuid, -1); // Unlimited uses

    // First access - granted
    auto r1 = validateAccessEndpoint(uuid, 1);
    EXPECT_TRUE(r1.granted);
    EXPECT_EQ(r1.current_uses, 1);

    // Second access on different door - granted (unlimited uses)
    auto r2 = validateAccessEndpoint(uuid, 2);
    EXPECT_TRUE(r2.granted);
    EXPECT_EQ(r2.current_uses, 2);

    // Third access on yet another door - granted (unlimited uses)
    auto r3 = validateAccessEndpoint(uuid, 3);
    EXPECT_TRUE(r3.granted);
    EXPECT_EQ(r3.current_uses, 3);

    auto logs = db->getLogsByTicket(uuid);
    EXPECT_EQ(logs.size(), 3);
    EXPECT_EQ(db->getLogsByTicket(uuid).size(), 3);
}

// Test bulk ticket creation
TEST_F(APIEndpointTest, BulkCreate_MultipleTickets) {
    for (int i = 0; i < 100; i++) {
        char uuid[48];
        snprintf(uuid, sizeof(uuid), "550e8400-e29b-41d4-a716-%012d", i);
        auto response = createTicketEndpoint(uuid, (i % 2 == 0) ? -1 : 1); // Alternate unlimited and single use
        EXPECT_TRUE(response.success);
    }

    EXPECT_EQ(db->getTotalTickets(), 100);
    // As `getAvailableTickets` and `getUsedTickets` are correctly implemented in mock now,
    // we can check them
    EXPECT_EQ(db->getAvailableTickets(), 50); // 50 unlimited, 50 single-use (all available)
    EXPECT_EQ(db->getUsedTickets(), 0); // None are used yet
}

// Test concurrent-like access (sequential simulation)
TEST_F(APIEndpointTest, SequentialAccess_HighVolume) {
    // Create 50 tickets with 1 use each
    for (int i = 0; i < 50; i++) {
        char uuid[48];
        snprintf(uuid, sizeof(uuid), "550e8400-e29b-41d4-a716-%012d", i);
        db->createTicket(uuid, 1);
    }

    // Use all of them across 4 doors
    for (int i = 0; i < 50; i++) {
        char uuid[48];
        snprintf(uuid, sizeof(uuid), "550e8400-e29b-41d4-a716-%012d", i);
        int door_id = (i % 4) + 1;

        auto result = validateAccessEndpoint(uuid, door_id);
        EXPECT_TRUE(result.granted);
        EXPECT_EQ(result.current_uses, 1);
        EXPECT_EQ(result.max_uses, 1);
    }

    EXPECT_EQ(db->getUsedTickets(), 50); // All 50 are now used
    EXPECT_EQ(db->getGrantedCount(), 50);
}
