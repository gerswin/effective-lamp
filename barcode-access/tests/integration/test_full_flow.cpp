#include <gtest/gtest.h>
#include "mock_database.hpp"
#include "common.hpp"
#include <string>
#include <vector>
#include <random>
#include <algorithm>

using namespace mocks;

// Full system integration tests
// Simulates the complete flow from ticket creation to door access

class FullFlowTest : public ::testing::Test {
protected:
    MockDatabase* db;

    void SetUp() override {
        db = &MockDatabase::getInstance();
        db->reset();
    }

    void TearDown() override {
        db->reset();
    }

    // Generate a valid UUID
    std::string generateUUID() {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_int_distribution<> dis(0, 15);

        const char* hex = "0123456789abcdef";
        std::string uuid = "xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx";

        for (char& c : uuid) {
            if (c == 'x') {
                c = hex[dis(gen)];
            } else if (c == 'y') {
                c = hex[(dis(gen) & 0x3) | 0x8];
            }
        }

        return uuid;
    }

    // Simulate complete access validation
    struct AccessAttempt {
        std::string uuid;
        int door_id;
        bool granted;
        std::string reason;
    };

    AccessAttempt attemptAccess(const std::string& uuid, int door_id) {
        AccessAttempt attempt;
        attempt.uuid = uuid;
        attempt.door_id = door_id;

        // Validate UUID format
        if (!barcode_access::is_valid_ticket_code(uuid)) {
            attempt.granted = false;
            attempt.reason = "DENIED_INVALID_FORMAT";
            db->logAccess(uuid, door_id, false, attempt.reason);
            return attempt;
        }

        // Check ticket exists
        auto ticket = db->getTicket(uuid);
        if (!ticket.has_value()) {
            attempt.granted = false;
            attempt.reason = "DENIED_NOT_FOUND";
            db->logAccess(uuid, door_id, false, attempt.reason);
            return attempt;
        }

        // Check if already used
        if (ticket->used) {
            attempt.granted = false;
            attempt.reason = "DENIED_ALREADY_USED";
            db->logAccess(uuid, door_id, false, attempt.reason);
            return attempt;
        }

        // Grant access
        db->markTicketUsed(uuid, door_id);
        attempt.granted = true;
        attempt.reason = "GRANTED";
        db->logAccess(uuid, door_id, true, attempt.reason);

        return attempt;
    }
};

// Test: Complete event day simulation
TEST_F(FullFlowTest, EventDaySimulation_SmallEvent) {
    const int TOTAL_TICKETS = 100;
    const int EXPECTED_ATTENDANCE = 80;  // 80% show up

    // Phase 1: Pre-event - Load all tickets
    std::vector<std::string> tickets;
    for (int i = 0; i < TOTAL_TICKETS; i++) {
        std::string uuid = generateUUID();
        EXPECT_TRUE(db->createTicket(uuid));
        tickets.push_back(uuid);
    }
    EXPECT_EQ(db->getTotalTickets(), TOTAL_TICKETS);
    EXPECT_EQ(db->getAvailableTickets(), TOTAL_TICKETS);

    // Phase 2: Event - Attendees enter through doors
    int successfulEntries = 0;
    for (int i = 0; i < EXPECTED_ATTENDANCE; i++) {
        int door_id = (i % 4) + 1;  // Distribute across 4 doors
        auto attempt = attemptAccess(tickets[i], door_id);

        if (attempt.granted) {
            successfulEntries++;
        }
    }

    EXPECT_EQ(successfulEntries, EXPECTED_ATTENDANCE);
    EXPECT_EQ(db->getUsedTickets(), EXPECTED_ATTENDANCE);
    EXPECT_EQ(db->getAvailableTickets(), TOTAL_TICKETS - EXPECTED_ATTENDANCE);
    EXPECT_EQ(db->getGrantedCount(), EXPECTED_ATTENDANCE);
}

// Test: Duplicate entry attempts
TEST_F(FullFlowTest, DuplicateEntryAttempts) {
    std::string uuid = generateUUID();
    db->createTicket(uuid);

    // First entry - should succeed
    auto first = attemptAccess(uuid, 1);
    EXPECT_TRUE(first.granted);
    EXPECT_EQ(first.reason, "GRANTED");

    // Second entry - should fail
    auto second = attemptAccess(uuid, 1);
    EXPECT_FALSE(second.granted);
    EXPECT_EQ(second.reason, "DENIED_ALREADY_USED");

    // Third entry at different door - should still fail
    auto third = attemptAccess(uuid, 2);
    EXPECT_FALSE(third.granted);
    EXPECT_EQ(third.reason, "DENIED_ALREADY_USED");

    // Verify logs
    auto logs = db->getLogsByTicket(uuid);
    EXPECT_EQ(logs.size(), 3);

    // Check attempts count
    EXPECT_EQ(db->getAttemptCount(uuid), 3);
}

// Test: Invalid ticket attempts
TEST_F(FullFlowTest, InvalidTicketAttempts) {
    // Try non-existent ticket
    std::string fakeUUID = generateUUID();
    auto attempt1 = attemptAccess(fakeUUID, 1);
    EXPECT_FALSE(attempt1.granted);
    EXPECT_EQ(attempt1.reason, "DENIED_NOT_FOUND");

    // Try invalid format
    auto attempt2 = attemptAccess("invalid-format", 2);
    EXPECT_FALSE(attempt2.granted);
    EXPECT_EQ(attempt2.reason, "DENIED_INVALID_FORMAT");

    // Try empty UUID
    auto attempt3 = attemptAccess("", 3);
    EXPECT_FALSE(attempt3.granted);
    EXPECT_EQ(attempt3.reason, "DENIED_INVALID_FORMAT");

    EXPECT_EQ(db->getDeniedCount(), 3);
}

// Test: All four doors receive traffic
TEST_F(FullFlowTest, AllDoorsReceiveTraffic) {
    const int ENTRIES_PER_DOOR = 25;

    // Create tickets for each door
    for (int door = 1; door <= 4; door++) {
        for (int i = 0; i < ENTRIES_PER_DOOR; i++) {
            std::string uuid = generateUUID();
            db->createTicket(uuid);
            auto attempt = attemptAccess(uuid, door);
            EXPECT_TRUE(attempt.granted);
        }
    }

    // Verify each door has the expected entries
    for (int door = 1; door <= 4; door++) {
        auto doorLogs = db->getLogsByDoor(door);
        EXPECT_EQ(doorLogs.size(), ENTRIES_PER_DOOR);
    }

    EXPECT_EQ(db->getTotalAttempts(), 4 * ENTRIES_PER_DOOR);
    EXPECT_EQ(db->getGrantedCount(), 4 * ENTRIES_PER_DOOR);
}

// Test: Large scale simulation
TEST_F(FullFlowTest, LargeScaleSimulation_1000Tickets) {
    const int TICKET_COUNT = 1000;

    // Create all tickets
    std::vector<std::string> tickets;
    tickets.reserve(TICKET_COUNT);

    for (int i = 0; i < TICKET_COUNT; i++) {
        std::string uuid = generateUUID();
        db->createTicket(uuid);
        tickets.push_back(uuid);
    }

    EXPECT_EQ(db->getTotalTickets(), TICKET_COUNT);

    // Simulate 80% attendance with some retry attempts
    int attempts = 0;
    int granted = 0;
    int denied = 0;

    for (int i = 0; i < TICKET_COUNT * 0.8; i++) {
        int door_id = (i % 4) + 1;
        auto attempt = attemptAccess(tickets[i], door_id);
        attempts++;

        if (attempt.granted) {
            granted++;

            // Simulate some people trying to re-enter
            if (i % 10 == 0) {
                auto retry = attemptAccess(tickets[i], door_id);
                attempts++;
                if (!retry.granted) denied++;
            }
        }
    }

    EXPECT_EQ(granted, static_cast<int>(TICKET_COUNT * 0.8));
    EXPECT_EQ(db->getGrantedCount(), granted);
    EXPECT_GT(denied, 0);  // Some retries should have been denied
}

// Test: Mixed valid and invalid entries
TEST_F(FullFlowTest, MixedValidInvalidEntries) {
    // Create some valid tickets
    std::vector<std::string> validTickets;
    for (int i = 0; i < 50; i++) {
        std::string uuid = generateUUID();
        db->createTicket(uuid);
        validTickets.push_back(uuid);
    }

    // Generate some fake tickets (not in DB)
    std::vector<std::string> fakeTickets;
    for (int i = 0; i < 20; i++) {
        fakeTickets.push_back(generateUUID());
    }

    int validGranted = 0;
    int fakeDenied = 0;

    // Try valid tickets
    for (const auto& uuid : validTickets) {
        if (attemptAccess(uuid, 1).granted) {
            validGranted++;
        }
    }

    // Try fake tickets
    for (const auto& uuid : fakeTickets) {
        if (!attemptAccess(uuid, 1).granted) {
            fakeDenied++;
        }
    }

    EXPECT_EQ(validGranted, 50);
    EXPECT_EQ(fakeDenied, 20);
}

// Test: Statistics accuracy
TEST_F(FullFlowTest, StatisticsAccuracy) {
    // Create 100 tickets
    std::vector<std::string> tickets;
    for (int i = 0; i < 100; i++) {
        std::string uuid = generateUUID();
        db->createTicket(uuid);
        tickets.push_back(uuid);
    }

    // Use 60 tickets
    for (int i = 0; i < 60; i++) {
        attemptAccess(tickets[i], (i % 4) + 1);
    }

    // Try to reuse 10 of them
    for (int i = 0; i < 10; i++) {
        attemptAccess(tickets[i], 1);
    }

    // Try 5 fake tickets
    for (int i = 0; i < 5; i++) {
        attemptAccess(generateUUID(), 1);
    }

    // Verify statistics
    EXPECT_EQ(db->getTotalTickets(), 100);
    EXPECT_EQ(db->getUsedTickets(), 60);
    EXPECT_EQ(db->getAvailableTickets(), 40);
    EXPECT_EQ(db->getTotalAttempts(), 75);  // 60 + 10 + 5
    EXPECT_EQ(db->getGrantedCount(), 60);
    EXPECT_EQ(db->getDeniedCount(), 15);  // 10 reuse + 5 fake
}

// Test: Door load balancing verification
TEST_F(FullFlowTest, DoorLoadBalancing) {
    const int TOTAL_ENTRIES = 400;  // 100 per door

    // Create tickets
    std::vector<std::string> tickets;
    for (int i = 0; i < TOTAL_ENTRIES; i++) {
        std::string uuid = generateUUID();
        db->createTicket(uuid);
        tickets.push_back(uuid);
    }

    // Distribute evenly across doors
    for (int i = 0; i < TOTAL_ENTRIES; i++) {
        int door = (i % 4) + 1;
        attemptAccess(tickets[i], door);
    }

    // Verify even distribution
    for (int door = 1; door <= 4; door++) {
        auto logs = db->getLogsByDoor(door);
        EXPECT_EQ(logs.size(), 100);  // Each door should have 100 entries
    }
}

// Test: Sequential access pattern
TEST_F(FullFlowTest, SequentialAccessPattern) {
    // Simulate queue-like entry pattern
    std::vector<std::string> queue;
    for (int i = 0; i < 50; i++) {
        std::string uuid = generateUUID();
        db->createTicket(uuid);
        queue.push_back(uuid);
    }

    // People enter one by one
    for (size_t i = 0; i < queue.size(); i++) {
        auto attempt = attemptAccess(queue[i], 1);
        EXPECT_TRUE(attempt.granted);

        // Verify order in logs (most recent first)
        auto logs = db->getAllLogs();
        EXPECT_EQ(logs.size(), i + 1);
    }
}

// Test: Reset functionality
TEST_F(FullFlowTest, DatabaseReset) {
    // Add data
    for (int i = 0; i < 100; i++) {
        db->createTicket(generateUUID());
    }
    EXPECT_EQ(db->getTotalTickets(), 100);

    // Reset
    db->reset();

    // Verify clean state
    EXPECT_EQ(db->getTotalTickets(), 0);
    EXPECT_EQ(db->getTotalAttempts(), 0);
    EXPECT_TRUE(db->getAllTickets().empty());
    EXPECT_TRUE(db->getAllLogs().empty());
}
