cmake_minimum_required(VERSION 3.10)
project(ocmfet-server-feedback)

# Set C++ standard to C++20 (jthread is not available in C++17)
set(CMAKE_CXX_STANDARD 20)

add_executable(server src/main.cpp src/hw_peripherals.cpp src/acquirer.cpp src/server.cpp)

target_include_directories(server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

target_link_libraries(server PRIVATE bcm2835 pthread rt)