#include <gtest/gtest.h>
#include "mock_database.hpp"
#include "common.hpp"

using namespace mocks;

class TicketServiceTest : public ::testing::Test {
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

// Test ticket creation
TEST_F(TicketServiceTest, CreateTicket_ValidUUID) {
    std::string uuid = "550e8400-e29b-41d4-a716-446655440000";
    EXPECT_TRUE(db->createTicket(uuid));
    EXPECT_EQ(db->getTotalTickets(), 1);
}

TEST_F(TicketServiceTest, CreateTicket_MultipleTickets) {
    EXPECT_TRUE(db->createTicket("550e8400-e29b-41d4-a716-446655440001"));
    EXPECT_TRUE(db->createTicket("550e8400-e29b-41d4-a716-446655440002"));
    EXPECT_TRUE(db->createTicket("550e8400-e29b-41d4-a716-446655440003"));
    EXPECT_EQ(db->getTotalTickets(), 3);
}

TEST_F(TicketServiceTest, CreateTicket_DuplicateUUID) {
    std::string uuid = "550e8400-e29b-41d4-a716-446655440000";
    EXPECT_TRUE(db->createTicket(uuid));
    EXPECT_TRUE(db->createTicket(uuid)); // Should succeed (ON CONFLICT DO NOTHING)
    EXPECT_EQ(db->getTotalTickets(), 1); // Still only one ticket
}

// Test ticket retrieval
TEST_F(TicketServiceTest, GetTicket_Exists) {
    std::string uuid = "550e8400-e29b-41d4-a716-446655440000";
    db->createTicket(uuid);

    auto ticket = db->getTicket(uuid);
    ASSERT_TRUE(ticket.has_value());
    EXPECT_EQ(ticket->uuid, uuid);
    EXPECT_FALSE(ticket->used);
}

TEST_F(TicketServiceTest, GetTicket_NotExists) {
    auto ticket = db->getTicket("550e8400-e29b-41d4-a716-446655440000");
    EXPECT_FALSE(ticket.has_value());
}

TEST_F(TicketServiceTest, GetAllTickets_Empty) {
    auto tickets = db->getAllTickets();
    EXPECT_TRUE(tickets.empty());
}

TEST_F(TicketServiceTest, GetAllTickets_WithTickets) {
    db->createTicket("550e8400-e29b-41d4-a716-446655440001");
    db->createTicket("550e8400-e29b-41d4-a716-446655440002");

    auto tickets = db->getAllTickets();
    EXPECT_EQ(tickets.size(), 2);
}

TEST_F(TicketServiceTest, GetAllTickets_Pagination) {
    for (int i = 0; i < 10; i++) {
        std::string uuid = "550e8400-e29b-41d4-a716-44665544000" + std::to_string(i);
        db->createTicket(uuid);
    }

    auto page1 = db->getAllTickets(5, 0);
    EXPECT_EQ(page1.size(), 5);

    auto page2 = db->getAllTickets(5, 5);
    EXPECT_EQ(page2.size(), 5);

    auto page3 = db->getAllTickets(5, 10);
    EXPECT_TRUE(page3.empty());
}

// Test ticket deletion
TEST_F(TicketServiceTest, DeleteTicket_Exists) {
    std::string uuid = "550e8400-e29b-41d4-a716-446655440000";
    db->createTicket(uuid);
    EXPECT_EQ(db->getTotalTickets(), 1);

    EXPECT_TRUE(db->deleteTicket(uuid));
    EXPECT_EQ(db->getTotalTickets(), 0);
}

TEST_F(TicketServiceTest, DeleteTicket_NotExists) {
    EXPECT_FALSE(db->deleteTicket("550e8400-e29b-41d4-a716-446655440000"));
}

// Test ticket usage
TEST_F(TicketServiceTest, MarkTicketUsed_Success) {
    std::string uuid = "550e8400-e29b-41d4-a716-446655440000";
    db->createTicket(uuid);

    EXPECT_TRUE(db->markTicketUsed(uuid, 1));

    auto ticket = db->getTicket(uuid);
    ASSERT_TRUE(ticket.has_value());
    EXPECT_TRUE(ticket->used);
    EXPECT_EQ(ticket->used_at_door, 1);
    EXPECT_FALSE(ticket->used_at.empty());
}

TEST_F(TicketServiceTest, MarkTicketUsed_NotExists) {
    EXPECT_FALSE(db->markTicketUsed("550e8400-e29b-41d4-a716-446655440000", 1));
}

TEST_F(TicketServiceTest, MarkTicketUsed_DifferentDoors) {
    std::string uuid1 = "550e8400-e29b-41d4-a716-446655440001";
    std::string uuid2 = "550e8400-e29b-41d4-a716-446655440002";
    std::string uuid3 = "550e8400-e29b-41d4-a716-446655440003";
    std::string uuid4 = "550e8400-e29b-41d4-a716-446655440004";

    db->createTicket(uuid1);
    db->createTicket(uuid2);
    db->createTicket(uuid3);
    db->createTicket(uuid4);

    db->markTicketUsed(uuid1, 1);
    db->markTicketUsed(uuid2, 2);
    db->markTicketUsed(uuid3, 3);
    db->markTicketUsed(uuid4, 4);

    EXPECT_EQ(db->getTicket(uuid1)->used_at_door, 1);
    EXPECT_EQ(db->getTicket(uuid2)->used_at_door, 2);
    EXPECT_EQ(db->getTicket(uuid3)->used_at_door, 3);
    EXPECT_EQ(db->getTicket(uuid4)->used_at_door, 4);
}

// Test ticket statistics
TEST_F(TicketServiceTest, Stats_Empty) {
    EXPECT_EQ(db->getTotalTickets(), 0);
    EXPECT_EQ(db->getUsedTickets(), 0);
    EXPECT_EQ(db->getAvailableTickets(), 0);
}

TEST_F(TicketServiceTest, Stats_AllAvailable) {
    db->createTicket("550e8400-e29b-41d4-a716-446655440001");
    db->createTicket("550e8400-e29b-41d4-a716-446655440002");
    db->createTicket("550e8400-e29b-41d4-a716-446655440003");

    EXPECT_EQ(db->getTotalTickets(), 3);
    EXPECT_EQ(db->getUsedTickets(), 0);
    EXPECT_EQ(db->getAvailableTickets(), 3);
}

TEST_F(TicketServiceTest, Stats_SomeUsed) {
    db->createTicket("550e8400-e29b-41d4-a716-446655440001");
    db->createTicket("550e8400-e29b-41d4-a716-446655440002");
    db->createTicket("550e8400-e29b-41d4-a716-446655440003");

    db->markTicketUsed("550e8400-e29b-41d4-a716-446655440001", 1);

    EXPECT_EQ(db->getTotalTickets(), 3);
    EXPECT_EQ(db->getUsedTickets(), 1);
    EXPECT_EQ(db->getAvailableTickets(), 2);
}

TEST_F(TicketServiceTest, Stats_AllUsed) {
    db->createTicket("550e8400-e29b-41d4-a716-446655440001");
    db->createTicket("550e8400-e29b-41d4-a716-446655440002");

    db->markTicketUsed("550e8400-e29b-41d4-a716-446655440001", 1);
    db->markTicketUsed("550e8400-e29b-41d4-a716-446655440002", 2);

    EXPECT_EQ(db->getTotalTickets(), 2);
    EXPECT_EQ(db->getUsedTickets(), 2);
    EXPECT_EQ(db->getAvailableTickets(), 0);
}

// Test validation flow simulation
TEST_F(TicketServiceTest, ValidationFlow_Success) {
    std::string uuid = "550e8400-e29b-41d4-a716-446655440000";

    // Create ticket
    EXPECT_TRUE(db->createTicket(uuid));

    // Validate UUID format
    EXPECT_TRUE(barcode_access::is_valid_uuid(uuid));

    // Check ticket exists and not used
    auto ticket = db->getTicket(uuid);
    ASSERT_TRUE(ticket.has_value());
    EXPECT_FALSE(ticket->used);

    // Mark as used
    EXPECT_TRUE(db->markTicketUsed(uuid, 1));

    // Verify it's now used
    ticket = db->getTicket(uuid);
    EXPECT_TRUE(ticket->used);
}

TEST_F(TicketServiceTest, ValidationFlow_AlreadyUsed) {
    std::string uuid = "550e8400-e29b-41d4-a716-446655440000";

    db->createTicket(uuid);
    db->markTicketUsed(uuid, 1);

    // Try to use again - ticket exists but is already used
    auto ticket = db->getTicket(uuid);
    ASSERT_TRUE(ticket.has_value());
    EXPECT_TRUE(ticket->used);

    // In real implementation, this would return DENIED_ALREADY_USED
}

TEST_F(TicketServiceTest, ValidationFlow_NotFound) {
    std::string uuid = "550e8400-e29b-41d4-a716-446655440000";

    // Ticket doesn't exist
    auto ticket = db->getTicket(uuid);
    EXPECT_FALSE(ticket.has_value());

    // In real implementation, this would return DENIED_NOT_FOUND
}
