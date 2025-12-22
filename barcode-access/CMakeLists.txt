cmake_minimum_required(VERSION 3.16)
project(barcode-access VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler flags
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Wextra")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

# Find required packages
find_package(PkgConfig REQUIRED)
find_package(CURL REQUIRED)
find_package(Threads REQUIRED)

# PostgreSQL
pkg_check_modules(LIBPQ REQUIRED libpq)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/common/include)
include_directories(${LIBPQ_INCLUDE_DIRS})
include_directories(${CURL_INCLUDE_DIRS})

# Oat++ dependencies
find_package(oatpp 1.3.0 REQUIRED)
find_package(oatpp-postgresql 1.3.0 REQUIRED)

link_directories(/usr/local/lib/oatpp-1.3.0)

include_directories(${oatpp_INCLUDE_DIRS})
include_directories(${oatpp-postgresql_INCLUDE_DIRS})

# Google Test for unit testing
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)
# Prevent overriding the parent project's compiler/linker settings on Windows
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Enable testing
enable_testing()

# Subdirectories
add_subdirectory(server)
add_subdirectory(client)
add_subdirectory(tests)

# Install targets
install(TARGETS access-server access-client
    RUNTIME DESTINATION bin
)
