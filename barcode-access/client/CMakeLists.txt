set(CLIENT_SOURCES
    src/main.cpp
    src/barcode_reader.cpp
    src/hikvision_isapi.cpp
    src/db_client.cpp
)

add_executable(access-client ${CLIENT_SOURCES})

target_include_directories(access-client PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CURL_INCLUDE_DIRS}
)

target_link_libraries(access-client
    ${CURL_LIBRARIES}
    ${LIBPQ_LIBRARIES}
    Threads::Threads
    atomic
)

# Link evdev if available (for USB HID reading)
find_library(EVDEV_LIB evdev)
if(EVDEV_LIB)
    target_link_libraries(access-client ${EVDEV_LIB})
    target_compile_definitions(access-client PRIVATE HAS_EVDEV)
endif()
