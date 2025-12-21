#include <gtest/gtest.h>
#include "mock_database.hpp"
#include "common.hpp"

using namespace mocks;

class AccessLogServiceTest : public ::testing::Test {
protected:
    MockDatabase* db;

    void SetUp() override {
        db = &MockDatabase::getInstance();
        db->reset();
    }

    void TearDown() override {
        db->reset();
    }
};

// Test logging access attempts
TEST_F(AccessLogServiceTest, LogAccess_Granted) {
    std::string uuid = "550e8400-e29b-41d4-a716-446655440000";

    EXPECT_TRUE(db->logAccess(uuid, 1, true, "GRANTED"));
    EXPECT_EQ(db->getTotalAttempts(), 1);
    EXPECT_EQ(db->getGrantedCount(), 1);
    EXPECT_EQ(db->getDeniedCount(), 0);
}

TEST_F(AccessLogServiceTest, LogAccess_Denied) {
    std::string uuid = "550e8400-e29b-41d4-a716-446655440000";

    EXPECT_TRUE(db->logAccess(uuid, 1, false, "DENIED_NOT_FOUND"));
    EXPECT_EQ(db->getTotalAttempts(), 1);
    EXPECT_EQ(db->getGrantedCount(), 0);
    EXPECT_EQ(db->getDeniedCount(), 1);
}

TEST_F(AccessLogServiceTest, LogAccess_MultipleAttempts) {
    EXPECT_TRUE(db->logAccess("uuid1", 1, true, "GRANTED"));
    EXPECT_TRUE(db->logAccess("uuid2", 2, false, "DENIED_NOT_FOUND"));
    EXPECT_TRUE(db->logAccess("uuid3", 3, true, "GRANTED"));
    EXPECT_TRUE(db->logAccess("uuid4", 4, false, "DENIED_ALREADY_USED"));

    EXPECT_EQ(db->getTotalAttempts(), 4);
    EXPECT_EQ(db->getGrantedCount(), 2);
    EXPECT_EQ(db->getDeniedCount(), 2);
}

// Test retrieving logs
TEST_F(AccessLogServiceTest, GetAllLogs_Empty) {
    auto logs = db->getAllLogs();
    EXPECT_TRUE(logs.empty());
}

TEST_F(AccessLogServiceTest, GetAllLogs_WithLogs) {
    db->logAccess("uuid1", 1, true, "GRANTED");
    db->logAccess("uuid2", 2, false, "DENIED");

    auto logs = db->getAllLogs();
    EXPECT_EQ(logs.size(), 2);
}

TEST_F(AccessLogServiceTest, GetAllLogs_Pagination) {
    for (int i = 0; i < 10; i++) {
        db->logAccess("uuid" + std::to_string(i), 1, true, "GRANTED");
    }

    auto page1 = db->getAllLogs(5, 0);
    EXPECT_EQ(page1.size(), 5);

    auto page2 = db->getAllLogs(5, 5);
    EXPECT_EQ(page2.size(), 5);

    auto page3 = db->getAllLogs(5, 10);
    EXPECT_TRUE(page3.empty());
}

TEST_F(AccessLogServiceTest, GetAllLogs_OrderByNewest) {
    db->logAccess("uuid1", 1, true, "FIRST");
    db->logAccess("uuid2", 1, true, "SECOND");
    db->logAccess("uuid3", 1, true, "THIRD");

    auto logs = db->getAllLogs();
    ASSERT_EQ(logs.size(), 3);

    // Newest first
    EXPECT_EQ(logs[0].reason, "THIRD");
    EXPECT_EQ(logs[1].reason, "SECOND");
    EXPECT_EQ(logs[2].reason, "FIRST");
}

// Test filtering logs by door
TEST_F(AccessLogServiceTest, GetLogsByDoor_Empty) {
    auto logs = db->getLogsByDoor(1);
    EXPECT_TRUE(logs.empty());
}

TEST_F(AccessLogServiceTest, GetLogsByDoor_FilterCorrectly) {
    db->logAccess("uuid1", 1, true, "GRANTED");
    db->logAccess("uuid2", 2, true, "GRANTED");
    db->logAccess("uuid3", 1, true, "GRANTED");
    db->logAccess("uuid4", 3, true, "GRANTED");
    db->logAccess("uuid5", 1, false, "DENIED");

    auto door1Logs = db->getLogsByDoor(1);
    EXPECT_EQ(door1Logs.size(), 3);

    auto door2Logs = db->getLogsByDoor(2);
    EXPECT_EQ(door2Logs.size(), 1);

    auto door3Logs = db->getLogsByDoor(3);
    EXPECT_EQ(door3Logs.size(), 1);

    auto door4Logs = db->getLogsByDoor(4);
    EXPECT_TRUE(door4Logs.empty());
}

TEST_F(AccessLogServiceTest, GetLogsByDoor_AllFourDoors) {
    db->logAccess("uuid1", 1, true, "GRANTED");
    db->logAccess("uuid2", 2, true, "GRANTED");
    db->logAccess("uuid3", 3, true, "GRANTED");
    db->logAccess("uuid4", 4, true, "GRANTED");

    EXPECT_EQ(db->getLogsByDoor(1).size(), 1);
    EXPECT_EQ(db->getLogsByDoor(2).size(), 1);
    EXPECT_EQ(db->getLogsByDoor(3).size(), 1);
    EXPECT_EQ(db->getLogsByDoor(4).size(), 1);
}

// Test filtering logs by ticket
TEST_F(AccessLogServiceTest, GetLogsByTicket_Empty) {
    auto logs = db->getLogsByTicket("nonexistent-uuid");
    EXPECT_TRUE(logs.empty());
}

TEST_F(AccessLogServiceTest, GetLogsByTicket_SingleAttempt) {
    std::string uuid = "550e8400-e29b-41d4-a716-446655440000";
    db->logAccess(uuid, 1, true, "GRANTED");

    auto logs = db->getLogsByTicket(uuid);
    ASSERT_EQ(logs.size(), 1);
    EXPECT_EQ(logs[0].ticket_uuid, uuid);
    EXPECT_TRUE(logs[0].granted);
}

TEST_F(AccessLogServiceTest, GetLogsByTicket_MultipleAttempts) {
    std::string uuid = "550e8400-e29b-41d4-a716-446655440000";

    db->logAccess(uuid, 1, true, "GRANTED");
    db->logAccess(uuid, 2, false, "DENIED_ALREADY_USED");
    db->logAccess(uuid, 3, false, "DENIED_ALREADY_USED");

    auto logs = db->getLogsByTicket(uuid);
    EXPECT_EQ(logs.size(), 3);
}

// Test attempt counting
TEST_F(AccessLogServiceTest, GetAttemptCount_NoAttempts) {
    EXPECT_EQ(db->getAttemptCount("nonexistent-uuid"), 0);
}

TEST_F(AccessLogServiceTest, GetAttemptCount_SingleAttempt) {
    std::string uuid = "550e8400-e29b-41d4-a716-446655440000";
    db->logAccess(uuid, 1, true, "GRANTED");

    EXPECT_EQ(db->getAttemptCount(uuid), 1);
}

TEST_F(AccessLogServiceTest, GetAttemptCount_MultipleAttempts) {
    std::string uuid = "550e8400-e29b-41d4-a716-446655440000";

    db->logAccess(uuid, 1, true, "GRANTED");
    db->logAccess(uuid, 1, false, "DENIED");
    db->logAccess(uuid, 2, false, "DENIED");

    EXPECT_EQ(db->getAttemptCount(uuid), 3);
}

// Test statistics
TEST_F(AccessLogServiceTest, Stats_Initial) {
    EXPECT_EQ(db->getTotalAttempts(), 0);
    EXPECT_EQ(db->getGrantedCount(), 0);
    EXPECT_EQ(db->getDeniedCount(), 0);
}

TEST_F(AccessLogServiceTest, Stats_MixedResults) {
    db->logAccess("uuid1", 1, true, "GRANTED");
    db->logAccess("uuid2", 1, true, "GRANTED");
    db->logAccess("uuid3", 1, true, "GRANTED");
    db->logAccess("uuid4", 1, false, "DENIED_NOT_FOUND");
    db->logAccess("uuid5", 1, false, "DENIED_ALREADY_USED");

    EXPECT_EQ(db->getTotalAttempts(), 5);
    EXPECT_EQ(db->getGrantedCount(), 3);
    EXPECT_EQ(db->getDeniedCount(), 2);
}

// Test complete access flow
TEST_F(AccessLogServiceTest, CompleteFlow_SuccessfulAccess) {
    std::string uuid = "550e8400-e29b-41d4-a716-446655440000";
    int door_id = 1;

    // Create and use ticket
    db->createTicket(uuid);
    db->markTicketUsed(uuid, door_id);
    db->logAccess(uuid, door_id, true, "GRANTED");

    // Verify log
    auto logs = db->getLogsByTicket(uuid);
    ASSERT_EQ(logs.size(), 1);
    EXPECT_TRUE(logs[0].granted);
    EXPECT_EQ(logs[0].door_id, door_id);
}

TEST_F(AccessLogServiceTest, CompleteFlow_RepeatedAttempts) {
    std::string uuid = "550e8400-e29b-41d4-a716-446655440000";

    // First attempt - success
    db->createTicket(uuid);
    db->markTicketUsed(uuid, 1);
    db->logAccess(uuid, 1, true, "GRANTED");

    // Second attempt - denied (already used)
    db->logAccess(uuid, 1, false, "DENIED_ALREADY_USED");

    // Third attempt on different door - still denied
    db->logAccess(uuid, 2, false, "DENIED_ALREADY_USED");

    auto logs = db->getLogsByTicket(uuid);
    EXPECT_EQ(logs.size(), 3);
    EXPECT_EQ(db->getAttemptCount(uuid), 3);
}

// Test log content
TEST_F(AccessLogServiceTest, LogContent_Complete) {
    std::string uuid = "550e8400-e29b-41d4-a716-446655440000";

    db->logAccess(uuid, 2, true, "GRANTED");

    auto logs = db->getAllLogs();
    ASSERT_EQ(logs.size(), 1);

    const auto& log = logs[0];
    EXPECT_EQ(log.ticket_uuid, uuid);
    EXPECT_EQ(log.door_id, 2);
    EXPECT_TRUE(log.granted);
    EXPECT_EQ(log.reason, "GRANTED");
    EXPECT_EQ(log.attempts, 1);
    EXPECT_FALSE(log.scanned_at.empty());
    EXPECT_GT(log.id, 0);
}

// Test log IDs are sequential
TEST_F(AccessLogServiceTest, LogIDs_Sequential) {
    db->logAccess("uuid1", 1, true, "GRANTED");
    db->logAccess("uuid2", 1, true, "GRANTED");
    db->logAccess("uuid3", 1, true, "GRANTED");

    auto logs = db->getAllLogs(100, 0);

    // IDs should be sequential (newest first, so 3, 2, 1)
    EXPECT_EQ(logs[0].id, 3);
    EXPECT_EQ(logs[1].id, 2);
    EXPECT_EQ(logs[2].id, 1);
}
