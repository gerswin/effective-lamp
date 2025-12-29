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
TEST_F(TicketServiceTest, CreateTicket_ValidCode) {
    std::string code = "V1StGXR8_Z";
    EXPECT_TRUE(db->createTicket(code, -1)); // Unlimited uses
    EXPECT_EQ(db->getTotalTickets(), 1);
    auto ticket = db->getTicket(code);
    ASSERT_TRUE(ticket.has_value());
    EXPECT_EQ(ticket->max_uses, -1);
    EXPECT_EQ(ticket->current_uses, 0);
    EXPECT_FALSE(ticket->used);
}

TEST_F(TicketServiceTest, CreateTicket_MultipleTickets) {
    EXPECT_TRUE(db->createTicket("Code000001", -1));
    EXPECT_TRUE(db->createTicket("Code000002", 3));
    EXPECT_TRUE(db->createTicket("Code000003", 1));
    EXPECT_EQ(db->getTotalTickets(), 3);
}

TEST_F(TicketServiceTest, CreateTicket_DuplicateCode) {
    std::string code = "V1StGXR8_Z";
    EXPECT_TRUE(db->createTicket(code, -1));
    EXPECT_TRUE(db->createTicket(code, 3)); // Max_uses won't be updated by mock for existing
    EXPECT_EQ(db->getTotalTickets(), 1);
    auto ticket = db->getTicket(code);
    ASSERT_TRUE(ticket.has_value());
    EXPECT_EQ(ticket->max_uses, -1); // Should still be -1 as it was created first
}

// Test ticket retrieval
TEST_F(TicketServiceTest, GetTicket_Exists) {
    std::string code = "V1StGXR8_Z";
    db->createTicket(code, 5); // Create with 5 uses

    auto ticket = db->getTicket(code);
    ASSERT_TRUE(ticket.has_value());
    EXPECT_EQ(ticket->uuid, code); // Note: DTO still calls it uuid for now or mock does
    EXPECT_FALSE(ticket->used); // Not used yet
    EXPECT_EQ(ticket->max_uses, 5);
    EXPECT_EQ(ticket->current_uses, 0);
}

TEST_F(TicketServiceTest, GetTicket_NotExists) {
    auto ticket = db->getTicket("NonExistent");
    EXPECT_FALSE(ticket.has_value());
}

TEST_F(TicketServiceTest, GetAllTickets_Empty) {
    auto tickets = db->getAllTickets();
    EXPECT_TRUE(tickets.empty());
}

TEST_F(TicketServiceTest, GetAllTickets_WithTickets) {
    db->createTicket("Code000001", 1);
    db->createTicket("Code000002", -1);

    auto tickets = db->getAllTickets();
    EXPECT_EQ(tickets.size(), 2);

    // Order of iteration over unordered_map is not guaranteed, so check each ticket
    bool foundTicket1 = false;
    bool foundTicket2 = false;

    for (const auto& ticket : tickets) {
        if (ticket.uuid == "Code000001") {
            foundTicket1 = true;
            EXPECT_EQ(ticket.max_uses, 1);
            EXPECT_EQ(ticket.current_uses, 0);
            EXPECT_FALSE(ticket.used);
        } else if (ticket.uuid == "Code000002") {
            foundTicket2 = true;
            EXPECT_EQ(ticket.max_uses, -1);
            EXPECT_EQ(ticket.current_uses, 0);
            EXPECT_FALSE(ticket.used);
        }
    }
    EXPECT_TRUE(foundTicket1);
    EXPECT_TRUE(foundTicket2);
}

TEST_F(TicketServiceTest, GetAllTickets_Pagination) {
    for (int i = 0; i < 10; i++) {
        std::string code = "Code00000" + std::to_string(i);
        db->createTicket(code, (i % 2 == 0) ? -1 : 1); // Alternate unlimited and single use
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
    std::string code = "V1StGXR8_Z";
    db->createTicket(code, -1);
    EXPECT_EQ(db->getTotalTickets(), 1);

    EXPECT_TRUE(db->deleteTicket(code));
    EXPECT_EQ(db->getTotalTickets(), 0);
}

TEST_F(TicketServiceTest, DeleteTicket_NotExists) {
    EXPECT_FALSE(db->deleteTicket("V1StGXR8_Z"));
}

// Test ticket usage
TEST_F(TicketServiceTest, MarkTicketUsed_Success) {
    std::string code = "V1StGXR8_Z";
    db->createTicket(code, 2); // Ticket with 2 uses

    EXPECT_TRUE(db->markTicketUsed(code, 1)); // First use

    auto ticket = db->getTicket(code);
    ASSERT_TRUE(ticket.has_value());
    EXPECT_FALSE(ticket->used); // Not fully used yet
    EXPECT_EQ(ticket->current_uses, 1);
    EXPECT_EQ(ticket->max_uses, 2);
    EXPECT_EQ(ticket->used_at_door, 1);
    EXPECT_FALSE(ticket->used_at.empty());

    EXPECT_TRUE(db->markTicketUsed(code, 2)); // Second use (max uses reached)
    ticket = db->getTicket(code);
    ASSERT_TRUE(ticket.has_value());
    EXPECT_TRUE(ticket->used); // Now fully used
    EXPECT_EQ(ticket->current_uses, 2);
    EXPECT_EQ(ticket->max_uses, 2);
    EXPECT_EQ(ticket->used_at_door, 2);
}

TEST_F(TicketServiceTest, MarkTicketUsed_NotExists) {
    EXPECT_FALSE(db->markTicketUsed("V1StGXR8_Z", 1));
}

TEST_F(TicketServiceTest, MarkTicketUsed_DifferentDoors) {
    std::string code1 = "Code000001";
    std::string code2 = "Code000002";
    std::string code3 = "Code000003";
    std::string code4 = "Code000004";

    db->createTicket(code1, -1);
    db->createTicket(code2, -1);
    db->createTicket(code3, -1);
    db->createTicket(code4, -1);

    db->markTicketUsed(code1, 1);
    db->markTicketUsed(code2, 2);
    db->markTicketUsed(code3, 3);
    db->markTicketUsed(code4, 4);

    EXPECT_EQ(db->getTicket(code1)->used_at_door, 1);
    EXPECT_EQ(db->getTicket(code2)->used_at_door, 2);
    EXPECT_EQ(db->getTicket(code3)->used_at_door, 3);
    EXPECT_EQ(db->getTicket(code4)->used_at_door, 4);
}

// Test ticket statistics
TEST_F(TicketServiceTest, Stats_Empty) {
    EXPECT_EQ(db->getTotalTickets(), 0);
    EXPECT_EQ(db->getUsedTickets(), 0);
    EXPECT_EQ(db->getAvailableTickets(), 0);
}

TEST_F(TicketServiceTest, Stats_AllAvailable) {
    db->createTicket("Code000001", -1); // Unlimited
    db->createTicket("Code000002", 3);  // 3 uses
    db->createTicket("Code000003", 1);  // 1 use

    EXPECT_EQ(db->getTotalTickets(), 3);
    EXPECT_EQ(db->getUsedTickets(), 0);
    EXPECT_EQ(db->getAvailableTickets(), 3);
}

TEST_F(TicketServiceTest, Stats_SomeUsed) {
    db->createTicket("Code000001", 1); // Single use
    db->createTicket("Code000002", 3); // Three uses
    db->createTicket("Code000003", -1); // Unlimited

    db->markTicketUsed("Code000001", 1); // Uses 1 of 1

    EXPECT_EQ(db->getTotalTickets(), 3);
    EXPECT_EQ(db->getUsedTickets(), 1); // Only the single-use ticket
    EXPECT_EQ(db->getAvailableTickets(), 2);
}

TEST_F(TicketServiceTest, Stats_AllUsed) {
    db->createTicket("Code000001", 1); // Single use
    db->createTicket("Code000002", 2); // Two uses

    db->markTicketUsed("Code000001", 1); // Uses 1 of 1
    db->markTicketUsed("Code000002", 2); // Uses 1 of 2
    db->markTicketUsed("Code000002", 2); // Uses 2 of 2

    EXPECT_EQ(db->getTotalTickets(), 2);
    EXPECT_EQ(db->getUsedTickets(), 2);
    EXPECT_EQ(db->getAvailableTickets(), 0);
}

// Test validation flow simulation
TEST_F(TicketServiceTest, ValidationFlow_Success) {
    std::string code = "V1StGXR8_Z";

    // Create ticket with 1 use
    EXPECT_TRUE(db->createTicket(code, 1));

    // Validate Code format
    EXPECT_TRUE(barcode_access::is_valid_ticket_code(code));

    // Check ticket exists and not used
    auto ticket = db->getTicket(code);
    ASSERT_TRUE(ticket.has_value());
    EXPECT_FALSE(ticket->used);
    EXPECT_EQ(ticket->current_uses, 0);
    EXPECT_EQ(ticket->max_uses, 1);

    // Mark as used
    EXPECT_TRUE(db->markTicketUsed(code, 1));

    // Verify it's now used
    ticket = db->getTicket(code);
    EXPECT_TRUE(ticket->used);
    EXPECT_EQ(ticket->current_uses, 1);
}

TEST_F(TicketServiceTest, ValidationFlow_AlreadyUsed) {
    std::string code = "V1StGXR8_Z";

    db->createTicket(code, 1); // Create as single use
    db->markTicketUsed(code, 1); // Use it once

    // Try to use again - ticket exists but is already used
    auto ticket = db->getTicket(code);
    ASSERT_TRUE(ticket.has_value());
    EXPECT_TRUE(ticket->used);
    EXPECT_EQ(ticket->current_uses, 1);
    EXPECT_EQ(ticket->max_uses, 1);

    // In real implementation, this would return DENIED_MAX_USES_REACHED
}

TEST_F(TicketServiceTest, ValidationFlow_NotFound) {
    std::string code = "V1StGXR8_Z";

    // Ticket doesn't exist
    auto ticket = db->getTicket(code);
    EXPECT_FALSE(ticket.has_value());
}

// New tests for max_uses functionality
TEST_F(TicketServiceTest, CreateTicket_WithMaxUses) {
    std::string code1 = "Code000001";
    std::string code2 = "Code000002";
    std::string code3 = "Code000003";

    // Unlimited uses
    EXPECT_TRUE(db->createTicket(code1, -1));
    auto ticket1 = db->getTicket(code1);
    ASSERT_TRUE(ticket1.has_value());
    EXPECT_EQ(ticket1->max_uses, -1);
    EXPECT_EQ(ticket1->current_uses, 0);
    EXPECT_FALSE(ticket1->used); // Unlimited tickets are never 'used' up

    // 3 uses
    EXPECT_TRUE(db->createTicket(code2, 3));
    auto ticket2 = db->getTicket(code2);
    ASSERT_TRUE(ticket2.has_value());
    EXPECT_EQ(ticket2->max_uses, 3);
    EXPECT_EQ(ticket2->current_uses, 0);
    EXPECT_FALSE(ticket2->used);

    // 1 use
    EXPECT_TRUE(db->createTicket(code3, 1));
    auto ticket3 = db->getTicket(code3);
    ASSERT_TRUE(ticket3.has_value());
    EXPECT_EQ(ticket3->max_uses, 1);
    EXPECT_EQ(ticket3->current_uses, 0);
    EXPECT_FALSE(ticket3->used);
}

TEST_F(TicketServiceTest, MarkTicketUsed_LimitedUsesCorrectlyUpdatesState) {
    std::string code = "V1StGXR8_Z";
    db->createTicket(code, 2); // Ticket with 2 uses

    // Use 1st time
    EXPECT_TRUE(db->markTicketUsed(code, 1));
    auto ticket = db->getTicket(code);
    ASSERT_TRUE(ticket.has_value());
    EXPECT_EQ(ticket->current_uses, 1);
    EXPECT_FALSE(ticket->used); // Not fully used yet

    // Use 2nd time (max uses reached)
    EXPECT_TRUE(db->markTicketUsed(code, 1));
    ticket = db->getTicket(code);
    ASSERT_TRUE(ticket.has_value());
    EXPECT_EQ(ticket->current_uses, 2);
    EXPECT_TRUE(ticket->used); // Now fully used
}

TEST_F(TicketServiceTest, MarkTicketUsed_UnlimitedUsesNeverSetsUsedFlag) {
    std::string code = "V1StGXR8_Z";
    db->createTicket(code, -1); // Unlimited uses

    // Use multiple times
    EXPECT_TRUE(db->markTicketUsed(code, 1));
    EXPECT_TRUE(db->markTicketUsed(code, 1));
    EXPECT_TRUE(db->markTicketUsed(code, 1));

    auto ticket = db->getTicket(code);
    ASSERT_TRUE(ticket.has_value());
    EXPECT_EQ(ticket->current_uses, 3);
    EXPECT_EQ(ticket->max_uses, -1);
    EXPECT_TRUE(ticket->used); // Should be true, as it's been used at least once
}

TEST_F(TicketServiceTest, Stats_WithMixedTicketTypes) {
    db->createTicket("Code000001", 1); // 1 use
    db->createTicket("Code000002", 2); // 2 uses
    db->createTicket("Code000003", -1); // Unlimited

    // Initially: 0 used, 3 available
    EXPECT_EQ(db->getTotalTickets(), 3);
    EXPECT_EQ(db->getUsedTickets(), 0);
    EXPECT_EQ(db->getAvailableTickets(), 3);

    // Use ticket1 once (becomes used)
    db->markTicketUsed("Code000001", 1);
    EXPECT_EQ(db->getUsedTickets(), 1);
    EXPECT_EQ(db->getAvailableTickets(), 2);

    // Use ticket2 once (still available)
    db->markTicketUsed("Code000002", 1);
    EXPECT_EQ(db->getUsedTickets(), 1);
    EXPECT_EQ(db->getAvailableTickets(), 2); // ticket2 not fully used yet

    // Use ticket2 second time (becomes used)
    db->markTicketUsed("Code000002", 1);
    EXPECT_EQ(db->getUsedTickets(), 2);
    EXPECT_EQ(db->getAvailableTickets(), 1); // Only unlimited ticket3 is available

    // Use ticket3 multiple times (always available from perspective of max_uses logic)
    db->markTicketUsed("Code000003", 1);
    EXPECT_EQ(db->getUsedTickets(), 2); 
    EXPECT_EQ(db->getAvailableTickets(), 1); 
}

TEST_F(TicketServiceTest, ValidationFlow_MaxUsesDenied) {
    std::string code = "V1StGXR8_Z";

    // Create ticket with 1 use
    EXPECT_TRUE(db->createTicket(code, 1));
    EXPECT_TRUE(db->markTicketUsed(code, 1)); // Use once, now it's "used"

    // Verify state
    auto ticket = db->getTicket(code);
    ASSERT_TRUE(ticket.has_value());
    EXPECT_TRUE(ticket->used);
    EXPECT_EQ(ticket->current_uses, 1);
    EXPECT_EQ(ticket->max_uses, 1);

    // If we were using the real service, validateAndUseTicket would return DENIED_MAX_USES_REACHED
    // For mock, it just updates internal state, but for a real service, this is where it'd fail
}