# Tests CMakeLists.txt

# Unit tests executable
add_executable(unit_tests
    unit/test_uuid_validation.cpp
    unit/test_ticket_service.cpp
    unit/test_access_log_service.cpp
    unit/test_barcode_reader.cpp
    unit/test_hikvision_isapi.cpp
    mocks/mock_database.cpp
)

target_include_directories(unit_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/common/include
    ${CMAKE_SOURCE_DIR}/server/src
    ${CMAKE_SOURCE_DIR}/client/src
    ${CMAKE_CURRENT_SOURCE_DIR}/mocks
)

target_link_libraries(unit_tests
    GTest::gtest
    GTest::gtest_main
    GTest::gmock
    Threads::Threads
)

# Integration tests executable
add_executable(integration_tests
    integration/test_api_endpoints.cpp
    integration/test_full_flow.cpp
    mocks/mock_database.cpp
)

target_include_directories(integration_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/common/include
    ${CMAKE_SOURCE_DIR}/server/src
    ${CMAKE_SOURCE_DIR}/client/src
    ${CMAKE_CURRENT_SOURCE_DIR}/mocks
)

target_link_libraries(integration_tests
    GTest::gtest
    GTest::gtest_main
    GTest::gmock
    Threads::Threads
)

# Register tests with CTest
include(GoogleTest)
gtest_discover_tests(unit_tests)
gtest_discover_tests(integration_tests)

# Add custom target to run all tests
add_custom_target(run_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS unit_tests integration_tests
)
