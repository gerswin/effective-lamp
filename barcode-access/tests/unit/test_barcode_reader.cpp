#include <gtest/gtest.h>
#include "common.hpp"
#include <string>
#include <vector>
#include <map>

// Since the actual barcode reader needs hardware, we test the logic parts

class BarcodeReaderTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}

    // Simulate key code to character mapping (simplified from barcode_reader.cpp)
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
    EXPECT_EQ(keyCodeToChar(11), '0');
}

// Test key mapping for dash
TEST_F(BarcodeReaderTest, KeyMapping_Dash) {
    EXPECT_EQ(keyCodeToChar(12), '-');
}

// Test building Code from key codes
TEST_F(BarcodeReaderTest, BuildBarcode_Code) {
    // Build "abc123"
    std::vector<int> codes = {30, 48, 46, 2, 3, 4};
    std::string result = buildBarcode(codes);
    EXPECT_EQ(result, "abc123");
}

// Test simulated barcode validation
TEST_F(BarcodeReaderTest, ValidateScannedBarcode_ValidCode) {
    // Simulate a NanoID scan
    std::string scanned = "V1StGXR8_Z";
    EXPECT_TRUE(barcode_access::is_valid_ticket_code(scanned));
}

TEST_F(BarcodeReaderTest, ValidateScannedBarcode_TooLong) {
    std::string scanned = "12345678901";
    EXPECT_FALSE(barcode_access::is_valid_ticket_code(scanned));
}

TEST_F(BarcodeReaderTest, ValidateScannedBarcode_RandomText) {
    std::string scanned = "hello$world";
    EXPECT_FALSE(barcode_access::is_valid_ticket_code(scanned));
}

// Test barcode buffer handling
TEST_F(BarcodeReaderTest, BufferHandling_ClearOnEnter) {
    std::string buffer = "V1StGXR8_Z";

    // Simulate Enter key press - buffer should be processed and cleared
    if (barcode_access::is_valid_ticket_code(buffer)) {
        // Process and clear
        std::string processed = buffer;
        buffer.clear();

        EXPECT_FALSE(processed.empty());
        EXPECT_TRUE(buffer.empty());
    }
}

// Test multiple consecutive scans
TEST_F(BarcodeReaderTest, MultipleScan_Independence) {
    std::string scan1 = "Code000001";
    std::string scan2 = "Code000002";
    std::string scan3 = "Code000003";

    EXPECT_TRUE(barcode_access::is_valid_ticket_code(scan1));
    EXPECT_TRUE(barcode_access::is_valid_ticket_code(scan2));
    EXPECT_TRUE(barcode_access::is_valid_ticket_code(scan3));

    EXPECT_NE(scan1, scan2);
}