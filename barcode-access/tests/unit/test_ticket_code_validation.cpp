#include <gtest/gtest.h>
#include "common.hpp"

using namespace barcode_access;

class TicketCodeValidationTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

// Test valid Ticket Code formats (NanoID)
TEST_F(TicketCodeValidationTest, ValidCode_Standard) {
    EXPECT_TRUE(is_valid_ticket_code("V1StGXR8_Z"));
}

TEST_F(TicketCodeValidationTest, ValidCode_Alphanumeric) {
    EXPECT_TRUE(is_valid_ticket_code("abc123XYZ"));
}

TEST_F(TicketCodeValidationTest, ValidCode_WithDashAndUnderscore) {
    EXPECT_TRUE(is_valid_ticket_code("A-B_C123"));
}

TEST_F(TicketCodeValidationTest, ValidCode_Short) {
    EXPECT_TRUE(is_valid_ticket_code("A"));
}

TEST_F(TicketCodeValidationTest, ValidCode_MaxLength) {
    // 10 chars
    EXPECT_TRUE(is_valid_ticket_code("1234567890"));
}

// Test invalid Ticket Code formats
TEST_F(TicketCodeValidationTest, InvalidCode_Empty) {
    EXPECT_FALSE(is_valid_ticket_code(""));
}

TEST_F(TicketCodeValidationTest, InvalidCode_TooLong) {
    // 11 chars
    EXPECT_FALSE(is_valid_ticket_code("12345678901"));
}

TEST_F(TicketCodeValidationTest, InvalidCode_InvalidCharacters) {
    EXPECT_FALSE(is_valid_ticket_code("abc$123"));
    EXPECT_FALSE(is_valid_ticket_code("abc.123"));
    EXPECT_FALSE(is_valid_ticket_code("abc/123"));
    EXPECT_FALSE(is_valid_ticket_code("abc\\123"));
    EXPECT_FALSE(is_valid_ticket_code("abc 123")); // Space
}

// Test AccessResult enum to string conversion
TEST_F(TicketCodeValidationTest, AccessResultToString_Granted) {
    EXPECT_EQ(access_result_to_string(AccessResult::GRANTED), "GRANTED");
}

TEST_F(TicketCodeValidationTest, AccessResultToString_DeniedNotFound) {
    EXPECT_EQ(access_result_to_string(AccessResult::DENIED_NOT_FOUND), "DENIED_NOT_FOUND");
}

TEST_F(TicketCodeValidationTest, AccessResultToString_DeniedAlreadyUsed) {
    EXPECT_EQ(access_result_to_string(AccessResult::DENIED_ALREADY_USED), "DENIED_ALREADY_USED");
}

TEST_F(TicketCodeValidationTest, AccessResultToString_DeniedInvalidFormat) {
    EXPECT_EQ(access_result_to_string(AccessResult::DENIED_INVALID_FORMAT), "DENIED_INVALID_FORMAT");
}

TEST_F(TicketCodeValidationTest, AccessResultToString_ErrorDB) {
    EXPECT_EQ(access_result_to_string(AccessResult::ERROR_DB), "ERROR_DB");
}

TEST_F(TicketCodeValidationTest, AccessResultToString_ErrorDoor) {
    EXPECT_EQ(access_result_to_string(AccessResult::ERROR_DOOR), "ERROR_DOOR");
}

TEST_F(TicketCodeValidationTest, AccessResultToString_DeniedMaxUsesReached) {
    EXPECT_EQ(access_result_to_string(AccessResult::DENIED_MAX_USES_REACHED), "DENIED_MAX_USES_REACHED");
}

// Test timestamp generation
TEST_F(TicketCodeValidationTest, TimestampNotEmpty) {
    std::string ts = get_current_timestamp();
    EXPECT_FALSE(ts.empty());
}

TEST_F(TicketCodeValidationTest, TimestampFormat) {
    std::string ts = get_current_timestamp();
    // Format should be "YYYY-MM-DD HH:MM:SS" (19 characters)
    EXPECT_EQ(ts.length(), 19);
    EXPECT_EQ(ts[4], '-');
    EXPECT_EQ(ts[7], '-');
    EXPECT_EQ(ts[10], ' ');
    EXPECT_EQ(ts[13], ':');
    EXPECT_EQ(ts[16], ':');
}