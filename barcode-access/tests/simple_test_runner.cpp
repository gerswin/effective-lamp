#include <iostream>
#include <string>
#include <vector>
#include <cassert>
#include "common.hpp"

using namespace barcode_access;

#define ASSERT_TRUE(condition) \
    if (!(condition)) { \
        std::cerr << "Assertion failed: " << #condition << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
        exit(1); \
    }

#define ASSERT_FALSE(condition) \
    if (condition) { \
        std::cerr << "Assertion failed: !" << #condition << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
        exit(1); \
    }

#define ASSERT_EQ(a, b) \
    if ((a) != (b)) { \
        std::cerr << "Assertion failed: " << #a << " == " << #b << " (" << (a) << " != " << (b) << ") at " << __FILE__ << ":" << __LINE__ << std::endl; \
        exit(1); \
    }

void test_ticket_code_validation() {
    std::cout << "Testing ticket code validation..." << std::endl;
    
    // Valid codes
    ASSERT_TRUE(is_valid_ticket_code("V1StGXR8_Z"));
    ASSERT_TRUE(is_valid_ticket_code("abc123XYZ"));
    ASSERT_TRUE(is_valid_ticket_code("A-B_C123"));
    ASSERT_TRUE(is_valid_ticket_code("1234567890"));
    
    // Invalid codes
    ASSERT_FALSE(is_valid_ticket_code(""));
    ASSERT_FALSE(is_valid_ticket_code("12345678901")); // Too long
    ASSERT_FALSE(is_valid_ticket_code("abc$123"));   // Special char
    ASSERT_FALSE(is_valid_ticket_code("abc 123"));   // Space
    
    std::cout << "Ticket code validation tests passed!" << std::endl;
}

void test_access_result_strings() {
    std::cout << "Testing access result strings..." << std::endl;
    
    ASSERT_EQ(access_result_to_string(AccessResult::GRANTED), "GRANTED");
    ASSERT_EQ(access_result_to_string(AccessResult::DENIED_NOT_FOUND), "DENIED_NOT_FOUND");
    ASSERT_EQ(access_result_to_string(AccessResult::DENIED_INVALID_FORMAT), "DENIED_INVALID_FORMAT");
    
    std::cout << "Access result string tests passed!" << std::endl;
}

void test_nanoid_generation() {
    std::cout << "Testing NanoID generation..." << std::endl;
    
    for (int i = 0; i < 100; ++i) {
        std::string code = generate_nanoid(10);
        ASSERT_EQ(code.length(), 10);
        ASSERT_TRUE(is_valid_ticket_code(code));
    }
    
    // Check different lengths
    ASSERT_EQ(generate_nanoid(5).length(), 5);
    ASSERT_EQ(generate_nanoid(20).length(), 20);
    
    std::cout << "NanoID generation tests passed!" << std::endl;
}

int main() {
    std::cout << "Running simple tests..." << std::endl;
    
    test_ticket_code_validation();
    test_access_result_strings();
    test_nanoid_generation();
    
    std::cout << "All simple tests passed!" << std::endl;
    return 0;
}