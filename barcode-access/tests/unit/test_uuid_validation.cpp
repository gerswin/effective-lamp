#include <gtest/gtest.h>
#include "common.hpp"

using namespace barcode_access;

class UUIDValidationTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

// Test valid UUID formats
TEST_F(UUIDValidationTest, ValidUUID_Standard) {
    EXPECT_TRUE(is_valid_uuid("550e8400-e29b-41d4-a716-446655440000"));
}

TEST_F(UUIDValidationTest, ValidUUID_AllLowercase) {
    EXPECT_TRUE(is_valid_uuid("6ba7b810-9dad-11d1-80b4-00c04fd430c8"));
}

TEST_F(UUIDValidationTest, ValidUUID_AllUppercase) {
    EXPECT_TRUE(is_valid_uuid("6BA7B810-9DAD-11D1-80B4-00C04FD430C8"));
}

TEST_F(UUIDValidationTest, ValidUUID_MixedCase) {
    EXPECT_TRUE(is_valid_uuid("6Ba7B810-9DaD-11D1-80b4-00C04fd430C8"));
}

TEST_F(UUIDValidationTest, ValidUUID_Version4) {
    EXPECT_TRUE(is_valid_uuid("f47ac10b-58cc-4372-a567-0e02b2c3d479"));
}

TEST_F(UUIDValidationTest, ValidUUID_AllZeros) {
    EXPECT_TRUE(is_valid_uuid("00000000-0000-0000-0000-000000000000"));
}

TEST_F(UUIDValidationTest, ValidUUID_AllFs) {
    EXPECT_TRUE(is_valid_uuid("ffffffff-ffff-ffff-ffff-ffffffffffff"));
}

// Test invalid UUID formats
TEST_F(UUIDValidationTest, InvalidUUID_Empty) {
    EXPECT_FALSE(is_valid_uuid(""));
}

TEST_F(UUIDValidationTest, InvalidUUID_TooShort) {
    EXPECT_FALSE(is_valid_uuid("550e8400-e29b-41d4-a716"));
}

TEST_F(UUIDValidationTest, InvalidUUID_TooLong) {
    EXPECT_FALSE(is_valid_uuid("550e8400-e29b-41d4-a716-4466554400001"));
}

TEST_F(UUIDValidationTest, InvalidUUID_NoDashes) {
    EXPECT_FALSE(is_valid_uuid("550e8400e29b41d4a716446655440000"));
}

TEST_F(UUIDValidationTest, InvalidUUID_WrongDashPosition) {
    EXPECT_FALSE(is_valid_uuid("550e840-0e29b-41d4-a716-446655440000"));
}

TEST_F(UUIDValidationTest, InvalidUUID_InvalidCharacter) {
    EXPECT_FALSE(is_valid_uuid("550e8400-e29b-41d4-a716-44665544000g"));
}

TEST_F(UUIDValidationTest, InvalidUUID_SpaceInMiddle) {
    EXPECT_FALSE(is_valid_uuid("550e8400-e29b-41d4 a716-446655440000"));
}

TEST_F(UUIDValidationTest, InvalidUUID_LeadingSpace) {
    EXPECT_FALSE(is_valid_uuid(" 550e8400-e29b-41d4-a716-446655440000"));
}

TEST_F(UUIDValidationTest, InvalidUUID_TrailingSpace) {
    EXPECT_FALSE(is_valid_uuid("550e8400-e29b-41d4-a716-446655440000 "));
}

TEST_F(UUIDValidationTest, InvalidUUID_Braces) {
    EXPECT_FALSE(is_valid_uuid("{550e8400-e29b-41d4-a716-446655440000}"));
}

TEST_F(UUIDValidationTest, InvalidUUID_SpecialCharacters) {
    EXPECT_FALSE(is_valid_uuid("550e8400-e29b-41d4-a716-44665544000!"));
}

TEST_F(UUIDValidationTest, InvalidUUID_RandomString) {
    EXPECT_FALSE(is_valid_uuid("not-a-valid-uuid-string"));
}

TEST_F(UUIDValidationTest, InvalidUUID_NumericOnly) {
    EXPECT_FALSE(is_valid_uuid("12345678901234567890123456789012"));
}

// Test AccessResult enum to string conversion
TEST_F(UUIDValidationTest, AccessResultToString_Granted) {
    EXPECT_EQ(access_result_to_string(AccessResult::GRANTED), "GRANTED");
}

TEST_F(UUIDValidationTest, AccessResultToString_DeniedNotFound) {
    EXPECT_EQ(access_result_to_string(AccessResult::DENIED_NOT_FOUND), "DENIED_NOT_FOUND");
}

TEST_F(UUIDValidationTest, AccessResultToString_DeniedAlreadyUsed) {
    EXPECT_EQ(access_result_to_string(AccessResult::DENIED_ALREADY_USED), "DENIED_ALREADY_USED");
}

TEST_F(UUIDValidationTest, AccessResultToString_DeniedInvalidUUID) {
    EXPECT_EQ(access_result_to_string(AccessResult::DENIED_INVALID_FORMAT), "DENIED_INVALID_FORMAT");
}

TEST_F(UUIDValidationTest, AccessResultToString_ErrorDB) {
    EXPECT_EQ(access_result_to_string(AccessResult::ERROR_DB), "ERROR_DB");
}

TEST_F(UUIDValidationTest, AccessResultToString_ErrorDoor) {
    EXPECT_EQ(access_result_to_string(AccessResult::ERROR_DOOR), "ERROR_DOOR");
}

TEST_F(UUIDValidationTest, AccessResultToString_DeniedMaxUsesReached) {
    EXPECT_EQ(access_result_to_string(AccessResult::DENIED_MAX_USES_REACHED), "DENIED_MAX_USES_REACHED");
}

// Test timestamp generation
TEST_F(UUIDValidationTest, TimestampNotEmpty) {
    std::string ts = get_current_timestamp();
    EXPECT_FALSE(ts.empty());
}

TEST_F(UUIDValidationTest, TimestampFormat) {
    std::string ts = get_current_timestamp();
    // Format should be "YYYY-MM-DD HH:MM:SS" (19 characters)
    EXPECT_EQ(ts.length(), 19);
    EXPECT_EQ(ts[4], '-');
    EXPECT_EQ(ts[7], '-');
    EXPECT_EQ(ts[10], ' ');
    EXPECT_EQ(ts[13], ':');
    EXPECT_EQ(ts[16], ':');
}
