cmake_minimum_required(VERSION 3.21)
project(test_cases)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -fcoroutines")

# Get real root of the project
get_filename_component(PROJECT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)

# Install databento
include(FetchContent)
FetchContent_Declare(
  databento
  GIT_REPOSITORY https://github.com/databento/databento-cpp.git
  GIT_TAG        main
)
FetchContent_MakeAvailable(databento)

find_package(GTest REQUIRED)
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(spdlog REQUIRED)

include_directories("/usr/local/lib")
include_directories(${LIBMONGOCXX_INCLUDE_DIR})
include_directories(${LIBBSONCXX_INCLUDE_DIR})

# Add files from the core directory
file(GLOB_RECURSE HEADER_FILES CONFIGURE_DEPENDS
    ${PROJECT_ROOT}/core/*.h
    ${PROJECT_ROOT}/core/*.hpp
    ${PROJECT_ROOT}/src/pnl/*.h
    ${PROJECT_ROOT}/src/pnl/*.hpp
)
file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS
    ${PROJECT_ROOT}/core/*.c
    ${PROJECT_ROOT}/core/*.cpp
    ${PROJECT_ROOT}/src/pnl/*.c
    ${PROJECT_ROOT}/src/pnl/*.cpp
)

# Test sources
file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/core/**/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

enable_testing()

add_executable(${PROJECT_NAME}
    ${TEST_SOURCES}
    ${SRC_FILES}
    ${HEADER_FILES}
)

# Include directories
target_include_directories(${PROJECT_NAME}
    PRIVATE
    ${PROJECT_ROOT}/core
    ${PROJECT_ROOT}/src
)

target_link_libraries(${PROJECT_NAME})
target_link_libraries(${PROJECT_NAME} PRIVATE databento::databento)
target_link_libraries(${PROJECT_NAME} PRIVATE OpenSSL::Crypto OpenSSL::SSL)
target_link_libraries(${PROJECT_NAME} PRIVATE spdlog::spdlog)
target_link_libraries(${PROJECT_NAME} PRIVATE GTest::gtest GTest::gtest_main Threads::Threads)

include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME})