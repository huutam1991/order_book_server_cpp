cmake_minimum_required(VERSION 3.21)
project(order_book_server_cpp)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -fcoroutines")

find_package(OpenSSL REQUIRED)
find_package(spdlog REQUIRED)
find_package(databento REQUIRED)

include_directories("/usr/local/lib")
include_directories(core)
include_directories(src)

# Header files
file (GLOB_RECURSE HEADER_FILES CONFIGURE_DEPENDS
    ${PROJECT_SOURCE_DIR}/core/*.h
    ${PROJECT_SOURCE_DIR}/core/*.hpp
    ${PROJECT_SOURCE_DIR}/src/*.h
    ${PROJECT_SOURCE_DIR}/src/*.hpp
)

# Source files
file (GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS
    ${PROJECT_SOURCE_DIR}/core/*.c
    ${PROJECT_SOURCE_DIR}/core/*.cpp
    ${PROJECT_SOURCE_DIR}/src/*.c
    ${PROJECT_SOURCE_DIR}/src/*.cpp
)

add_executable(${PROJECT_NAME} ${HEADER_FILES} ${SRC_FILES})

target_link_libraries(${PROJECT_NAME})
target_link_libraries(${PROJECT_NAME} PRIVATE databento::databento)
target_link_libraries(${PROJECT_NAME} PRIVATE OpenSSL::Crypto OpenSSL::SSL)
target_link_libraries(${PROJECT_NAME} PRIVATE spdlog::spdlog)