set(SERVER_SOURCES
    src/main.cpp
    src/db/database.cpp
    src/service/ticket_service.cpp
    src/service/access_log_service.cpp
    src/controller/ticket_controller.cpp
    src/controller/access_controller.cpp
    src/controller/static_controller.cpp
)

add_executable(access-server ${SERVER_SOURCES})

target_include_directories(access-server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${oatpp_INCLUDE_DIRS}
    ${oatpp-postgresql_INCLUDE_DIRS}
)

target_link_libraries(access-server
    oatpp
    oatpp-postgresql
    ${LIBPQ_LIBRARIES}
    Threads::Threads
)

# Copy web files to build directory
file(COPY ${CMAKE_SOURCE_DIR}/web DESTINATION ${CMAKE_BINARY_DIR})
