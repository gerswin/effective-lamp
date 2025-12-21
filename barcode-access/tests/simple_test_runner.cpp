/**
 * Simple Test Runner - No external dependencies
 * This file tests the core logic without requiring GoogleTest
 */

#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <stdexcept>
#include "mock_database.hpp"
#include "common.hpp"

using namespace mocks;
using namespace barcode_access;

// Simple test framework
int passed_count = 0;
int failed_count = 0;

void run_test(const char* name, std::function<void()> test_fn) {
    try {
        test_fn();
        std::cout << "[PASS] " << name << std::endl;
        passed_count++;
    } catch (const std::exception& e) {
        std::cout << "[FAIL] " << name << ": " << e.what() << std::endl;
        failed_count++;
    } catch (...) {
        std::cout << "[FAIL] " << name << ": Unknown exception" << std::endl;
        failed_count++;
    }
}

#define TEST(name) run_test(#name, []()
#define END_TEST );

#define ASSERT_TRUE(x) if (!(x)) throw std::runtime_error("ASSERT_TRUE failed: " #x)
#define ASSERT_FALSE(x) if (x) throw std::runtime_error("ASSERT_FALSE failed: " #x)
#define ASSERT_EQ(a, b) if ((a) != (b)) throw std::runtime_error("ASSERT_EQ failed")

int main() {
    std::cout << "============================================" << std::endl;
    std::cout << "  Barcode Access Control - Test Suite" << std::endl;
    std::cout << "============================================" << std::endl;
    std::cout << std::endl;

    // ============================================
    // UUID Validation Tests
    // ============================================
    std::cout << "--- UUID Validation Tests ---" << std::endl;

    TEST(UUID_ValidStandard) {
        ASSERT_TRUE(is_valid_uuid("550e8400-e29b-41d4-a716-446655440000"));
    } END_TEST

    TEST(UUID_ValidLowercase) {
        ASSERT_TRUE(is_valid_uuid("6ba7b810-9dad-11d1-80b4-00c04fd430c8"));
    } END_TEST

    TEST(UUID_ValidUppercase) {
        ASSERT_TRUE(is_valid_uuid("6BA7B810-9DAD-11D1-80B4-00C04FD430C8"));
    } END_TEST

    TEST(UUID_ValidMixedCase) {
        ASSERT_TRUE(is_valid_uuid("6Ba7B810-9DaD-11D1-80b4-00C04fd430C8"));
    } END_TEST

    TEST(UUID_InvalidEmpty) {
        ASSERT_FALSE(is_valid_uuid(""));
    } END_TEST

    TEST(UUID_InvalidTooShort) {
        ASSERT_FALSE(is_valid_uuid("550e8400-e29b-41d4-a716"));
    } END_TEST

    TEST(UUID_InvalidNoDashes) {
        ASSERT_FALSE(is_valid_uuid("550e8400e29b41d4a716446655440000"));
    } END_TEST

    TEST(UUID_InvalidCharacter) {
        ASSERT_FALSE(is_valid_uuid("550e8400-e29b-41d4-a716-44665544000g"));
    } END_TEST

    TEST(UUID_InvalidRandomString) {
        ASSERT_FALSE(is_valid_uuid("not-a-valid-uuid-string"));
    } END_TEST

    // ============================================
    // AccessResult Conversion Tests
    // ============================================
    std::cout << std::endl << "--- AccessResult Tests ---" << std::endl;

    TEST(AccessResult_Granted) {
        ASSERT_EQ(access_result_to_string(AccessResult::GRANTED), "GRANTED");
    } END_TEST

    TEST(AccessResult_DeniedNotFound) {
        ASSERT_EQ(access_result_to_string(AccessResult::DENIED_NOT_FOUND), "DENIED_NOT_FOUND");
    } END_TEST

    TEST(AccessResult_DeniedAlreadyUsed) {
        ASSERT_EQ(access_result_to_string(AccessResult::DENIED_ALREADY_USED), "DENIED_ALREADY_USED");
    } END_TEST

    // ============================================
    // Timestamp Tests
    // ============================================
    std::cout << std::endl << "--- Timestamp Tests ---" << std::endl;

    TEST(Timestamp_NotEmpty) {
        std::string ts = get_current_timestamp();
        ASSERT_FALSE(ts.empty());
    } END_TEST

    TEST(Timestamp_CorrectLength) {
        std::string ts = get_current_timestamp();
        ASSERT_EQ(ts.length(), static_cast<size_t>(19));
    } END_TEST

    // ============================================
    // Mock Database - Ticket Tests
    // ============================================
    std::cout << std::endl << "--- Mock Database Ticket Tests ---" << std::endl;

    TEST(MockDB_CreateTicket) {
        auto& db = MockDatabase::getInstance();
        db.reset();
        ASSERT_TRUE(db.createTicket("550e8400-e29b-41d4-a716-446655440000"));
        ASSERT_EQ(db.getTotalTickets(), 1);
        db.reset();
    } END_TEST

    TEST(MockDB_CreateMultipleTickets) {
        auto& db = MockDatabase::getInstance();
        db.reset();
        db.createTicket("550e8400-e29b-41d4-a716-446655440001");
        db.createTicket("550e8400-e29b-41d4-a716-446655440002");
        db.createTicket("550e8400-e29b-41d4-a716-446655440003");
        ASSERT_EQ(db.getTotalTickets(), 3);
        db.reset();
    } END_TEST

    TEST(MockDB_GetTicket) {
        auto& db = MockDatabase::getInstance();
        db.reset();
        std::string uuid = "550e8400-e29b-41d4-a716-446655440000";
        db.createTicket(uuid);
        auto ticket = db.getTicket(uuid);
        ASSERT_TRUE(ticket.has_value());
        ASSERT_EQ(ticket->uuid, uuid);
        ASSERT_FALSE(ticket->used);
        db.reset();
    } END_TEST

    TEST(MockDB_GetTicketNotExists) {
        auto& db = MockDatabase::getInstance();
        db.reset();
        auto ticket = db.getTicket("nonexistent-uuid");
        ASSERT_FALSE(ticket.has_value());
        db.reset();
    } END_TEST

    TEST(MockDB_DeleteTicket) {
        auto& db = MockDatabase::getInstance();
        db.reset();
        std::string uuid = "550e8400-e29b-41d4-a716-446655440000";
        db.createTicket(uuid);
        ASSERT_EQ(db.getTotalTickets(), 1);
        ASSERT_TRUE(db.deleteTicket(uuid));
        ASSERT_EQ(db.getTotalTickets(), 0);
        db.reset();
    } END_TEST

    TEST(MockDB_MarkTicketUsed) {
        auto& db = MockDatabase::getInstance();
        db.reset();
        std::string uuid = "550e8400-e29b-41d4-a716-446655440000";
        db.createTicket(uuid);
        ASSERT_TRUE(db.markTicketUsed(uuid, 1));
        auto ticket = db.getTicket(uuid);
        ASSERT_TRUE(ticket->used);
        ASSERT_EQ(ticket->used_at_door, 1);
        db.reset();
    } END_TEST

    TEST(MockDB_TicketStats) {
        auto& db = MockDatabase::getInstance();
        db.reset();
        db.createTicket("550e8400-e29b-41d4-a716-446655440001");
        db.createTicket("550e8400-e29b-41d4-a716-446655440002");
        db.createTicket("550e8400-e29b-41d4-a716-446655440003");
        ASSERT_EQ(db.getTotalTickets(), 3);
        ASSERT_EQ(db.getAvailableTickets(), 3);
        ASSERT_EQ(db.getUsedTickets(), 0);
        db.markTicketUsed("550e8400-e29b-41d4-a716-446655440001", 1);
        ASSERT_EQ(db.getUsedTickets(), 1);
        ASSERT_EQ(db.getAvailableTickets(), 2);
        db.reset();
    } END_TEST

    // ============================================
    // Mock Database - Access Log Tests
    // ============================================
    std::cout << std::endl << "--- Mock Database Log Tests ---" << std::endl;

    TEST(MockDB_LogAccess) {
        auto& db = MockDatabase::getInstance();
        db.reset();
        ASSERT_TRUE(db.logAccess("uuid-1", 1, true, "GRANTED"));
        ASSERT_EQ(db.getTotalAttempts(), 1);
        ASSERT_EQ(db.getGrantedCount(), 1);
        db.reset();
    } END_TEST

    TEST(MockDB_LogAccessDenied) {
        auto& db = MockDatabase::getInstance();
        db.reset();
        db.logAccess("uuid-1", 1, false, "DENIED");
        ASSERT_EQ(db.getDeniedCount(), 1);
        db.reset();
    } END_TEST

    TEST(MockDB_GetLogsByDoor) {
        auto& db = MockDatabase::getInstance();
        db.reset();
        db.logAccess("uuid-1", 1, true, "GRANTED");
        db.logAccess("uuid-2", 2, true, "GRANTED");
        db.logAccess("uuid-3", 1, true, "GRANTED");
        auto door1Logs = db.getLogsByDoor(1);
        ASSERT_EQ(door1Logs.size(), static_cast<size_t>(2));
        auto door2Logs = db.getLogsByDoor(2);
        ASSERT_EQ(door2Logs.size(), static_cast<size_t>(1));
        db.reset();
    } END_TEST

    TEST(MockDB_GetLogsByTicket) {
        auto& db = MockDatabase::getInstance();
        db.reset();
        std::string uuid = "550e8400-e29b-41d4-a716-446655440000";
        db.logAccess(uuid, 1, true, "GRANTED");
        db.logAccess(uuid, 1, false, "DENIED");
        auto logs = db.getLogsByTicket(uuid);
        ASSERT_EQ(logs.size(), static_cast<size_t>(2));
        db.reset();
    } END_TEST

    // ============================================
    // Full Flow Tests
    // ============================================
    std::cout << std::endl << "--- Full Flow Tests ---" << std::endl;

    TEST(Flow_SuccessfulAccess) {
        auto& db = MockDatabase::getInstance();
        db.reset();
        std::string uuid = "550e8400-e29b-41d4-a716-446655440000";
        ASSERT_TRUE(db.createTicket(uuid));
        ASSERT_TRUE(is_valid_uuid(uuid));
        auto ticket = db.getTicket(uuid);
        ASSERT_TRUE(ticket.has_value());
        ASSERT_FALSE(ticket->used);
        ASSERT_TRUE(db.markTicketUsed(uuid, 1));
        db.logAccess(uuid, 1, true, "GRANTED");
        ASSERT_EQ(db.getGrantedCount(), 1);
        db.reset();
    } END_TEST

    TEST(Flow_DeniedAlreadyUsed) {
        auto& db = MockDatabase::getInstance();
        db.reset();
        std::string uuid = "550e8400-e29b-41d4-a716-446655440000";
        db.createTicket(uuid);
        db.markTicketUsed(uuid, 1);
        auto ticket = db.getTicket(uuid);
        ASSERT_TRUE(ticket->used);
        db.logAccess(uuid, 1, false, "DENIED_ALREADY_USED");
        ASSERT_EQ(db.getDeniedCount(), 1);
        db.reset();
    } END_TEST

    TEST(Flow_MultipleDoors) {
        auto& db = MockDatabase::getInstance();
        db.reset();
        for (int i = 1; i <= 4; i++) {
            std::string uuid = "550e8400-e29b-41d4-a716-44665544000" + std::to_string(i);
            db.createTicket(uuid);
            db.markTicketUsed(uuid, i);
            db.logAccess(uuid, i, true, "GRANTED");
        }
        ASSERT_EQ(db.getTotalTickets(), 4);
        ASSERT_EQ(db.getUsedTickets(), 4);
        ASSERT_EQ(db.getGrantedCount(), 4);
        for (int i = 1; i <= 4; i++) {
            ASSERT_EQ(db.getLogsByDoor(i).size(), static_cast<size_t>(1));
        }
        db.reset();
    } END_TEST

    TEST(Flow_HighVolume) {
        auto& db = MockDatabase::getInstance();
        db.reset();
        const int COUNT = 100;
        for (int i = 0; i < COUNT; i++) {
            char uuid[48];
            snprintf(uuid, sizeof(uuid), "550e8400-e29b-41d4-a716-%012d", i);
            db.createTicket(uuid);
            db.markTicketUsed(uuid, (i % 4) + 1);
            db.logAccess(uuid, (i % 4) + 1, true, "GRANTED");
        }
        ASSERT_EQ(db.getTotalTickets(), COUNT);
        ASSERT_EQ(db.getUsedTickets(), COUNT);
        ASSERT_EQ(db.getGrantedCount(), COUNT);
        db.reset();
    } END_TEST

    // ============================================
    // Results
    // ============================================
    std::cout << std::endl;
    std::cout << "============================================" << std::endl;
    std::cout << "  Results: " << passed_count << " passed, " << failed_count << " failed" << std::endl;
    std::cout << "  Total: " << (passed_count + failed_count) << " tests" << std::endl;
    std::cout << "============================================" << std::endl;

    return failed_count > 0 ? 1 : 0;
}
