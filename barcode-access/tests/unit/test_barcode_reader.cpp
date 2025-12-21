#include <gtest/gtest.h>
#include "common.hpp"
#include <string>
#include <vector>

// Since the actual barcode reader needs hardware, we test the logic parts

class BarcodeReaderTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}

    // Simulate key code to character mapping (from barcode_reader.cpp)
    std::map<int, char> key_map = {
        {2, '1'}, {3, '2'}, {4, '3'}, {5, '4'}, {6, '5'},
        {7, '6'}, {8, '7'}, {9, '8'}, {10, '9'}, {11, '0'},
        {12, '-'},
        {30, 'a'}, {48, 'b'}, {46, 'c'}, {32, 'd'}, {18, 'e'},
        {33, 'f'}, {34, 'g'}, {35, 'h'}, {23, 'i'}, {36, 'j'},
        {37, 'k'}, {38, 'l'}, {50, 'm'}, {49, 'n'}, {24, 'o'},
        {25, 'p'}, {16, 'q'}, {19, 'r'}, {31, 's'}, {20, 't'},
        {22, 'u'}, {47, 'v'}, {17, 'w'}, {45, 'x'}, {21, 'y'},
        {44, 'z'}
    };

    char keyCodeToChar(int keyCode) {
        auto it = key_map.find(keyCode);
        return (it != key_map.end()) ? it->second : '\0';
    }

    std::string buildBarcode(const std::vector<int>& keyCodes) {
        std::string result;
        for (int code : keyCodes) {
            char c = keyCodeToChar(code);
            if (c != '\0') {
                result += c;
            }
        }
        return result;
    }
};

// Test key mapping for numbers
TEST_F(BarcodeReaderTest, KeyMapping_Numbers) {
    EXPECT_EQ(keyCodeToChar(2), '1');
    EXPECT_EQ(keyCodeToChar(3), '2');
    EXPECT_EQ(keyCodeToChar(4), '3');
    EXPECT_EQ(keyCodeToChar(5), '4');
    EXPECT_EQ(keyCodeToChar(6), '5');
    EXPECT_EQ(keyCodeToChar(7), '6');
    EXPECT_EQ(keyCodeToChar(8), '7');
    EXPECT_EQ(keyCodeToChar(9), '8');
    EXPECT_EQ(keyCodeToChar(10), '9');
    EXPECT_EQ(keyCodeToChar(11), '0');
}

// Test key mapping for dash
TEST_F(BarcodeReaderTest, KeyMapping_Dash) {
    EXPECT_EQ(keyCodeToChar(12), '-');
}

// Test key mapping for letters
TEST_F(BarcodeReaderTest, KeyMapping_Letters) {
    EXPECT_EQ(keyCodeToChar(30), 'a');
    EXPECT_EQ(keyCodeToChar(48), 'b');
    EXPECT_EQ(keyCodeToChar(46), 'c');
    EXPECT_EQ(keyCodeToChar(32), 'd');
    EXPECT_EQ(keyCodeToChar(18), 'e');
    EXPECT_EQ(keyCodeToChar(33), 'f');
}

// Test unknown key code
TEST_F(BarcodeReaderTest, KeyMapping_Unknown) {
    EXPECT_EQ(keyCodeToChar(999), '\0');
    EXPECT_EQ(keyCodeToChar(0), '\0');
    EXPECT_EQ(keyCodeToChar(-1), '\0');
}

// Test building UUID from key codes
TEST_F(BarcodeReaderTest, BuildBarcode_UUID) {
    // Build "550e8400" part
    std::vector<int> codes = {6, 6, 11, 18, 9, 5, 11, 11};
    std::string result = buildBarcode(codes);
    EXPECT_EQ(result, "550e8400");
}

// Test building UUID with dash
TEST_F(BarcodeReaderTest, BuildBarcode_UUIDWithDash) {
    // Build "550e8400-e29b"
    std::vector<int> codes = {6, 6, 11, 18, 9, 5, 11, 11, 12, 18, 3, 10, 48};
    std::string result = buildBarcode(codes);
    EXPECT_EQ(result, "550e8400-e29b");
}

// Test empty key sequence
TEST_F(BarcodeReaderTest, BuildBarcode_Empty) {
    std::vector<int> codes = {};
    std::string result = buildBarcode(codes);
    EXPECT_TRUE(result.empty());
}

// Test all hex characters
TEST_F(BarcodeReaderTest, BuildBarcode_HexCharacters) {
    // 0-9 and a-f should all be mappable
    std::vector<int> codes = {11, 2, 3, 4, 5, 6, 7, 8, 9, 10, 30, 48, 46, 32, 18, 33};
    std::string result = buildBarcode(codes);
    EXPECT_EQ(result, "0123456789abcdef");
}

// Test simulated barcode validation
TEST_F(BarcodeReaderTest, ValidateScannedBarcode_ValidUUID) {
    // Simulate a complete UUID scan
    std::string scanned = "550e8400-e29b-41d4-a716-446655440000";
    EXPECT_TRUE(barcode_access::is_valid_uuid(scanned));
}

TEST_F(BarcodeReaderTest, ValidateScannedBarcode_PartialUUID) {
    // Incomplete scan
    std::string scanned = "550e8400-e29b-41d4";
    EXPECT_FALSE(barcode_access::is_valid_uuid(scanned));
}

TEST_F(BarcodeReaderTest, ValidateScannedBarcode_RandomText) {
    std::string scanned = "hello world";
    EXPECT_FALSE(barcode_access::is_valid_uuid(scanned));
}

// Test device path validation
TEST_F(BarcodeReaderTest, DevicePath_Format) {
    std::string path1 = "/dev/input/event0";
    std::string path2 = "/dev/input/event1";
    std::string path3 = "/dev/input/by-id/usb-barcode-scanner";

    // Just verify path format (actual device testing requires hardware)
    EXPECT_EQ(path1.substr(0, 11), "/dev/input/");
    EXPECT_EQ(path2.substr(0, 11), "/dev/input/");
    EXPECT_EQ(path3.substr(0, 11), "/dev/input/");
}

// Test scan rate simulation
TEST_F(BarcodeReaderTest, ScanRate_UUIDCharacterCount) {
    std::string uuid = "550e8400-e29b-41d4-a716-446655440000";
    // UUID has 36 characters (32 hex + 4 dashes)
    EXPECT_EQ(uuid.length(), 36);

    // Count characters that need key mapping
    int hexCount = 0;
    int dashCount = 0;
    for (char c : uuid) {
        if (c == '-') dashCount++;
        else hexCount++;
    }
    EXPECT_EQ(hexCount, 32);
    EXPECT_EQ(dashCount, 4);
}

// Test barcode buffer handling
TEST_F(BarcodeReaderTest, BufferHandling_ClearOnEnter) {
    std::string buffer = "550e8400-e29b-41d4-a716-446655440000";

    // Simulate Enter key press - buffer should be processed and cleared
    if (barcode_access::is_valid_uuid(buffer)) {
        // Process and clear
        std::string processed = buffer;
        buffer.clear();

        EXPECT_FALSE(processed.empty());
        EXPECT_TRUE(buffer.empty());
    }
}

// Test multiple consecutive scans
TEST_F(BarcodeReaderTest, MultipleScan_Independence) {
    std::string scan1 = "550e8400-e29b-41d4-a716-446655440001";
    std::string scan2 = "550e8400-e29b-41d4-a716-446655440002";
    std::string scan3 = "550e8400-e29b-41d4-a716-446655440003";

    EXPECT_TRUE(barcode_access::is_valid_uuid(scan1));
    EXPECT_TRUE(barcode_access::is_valid_uuid(scan2));
    EXPECT_TRUE(barcode_access::is_valid_uuid(scan3));

    EXPECT_NE(scan1, scan2);
    EXPECT_NE(scan2, scan3);
    EXPECT_NE(scan1, scan3);
}
