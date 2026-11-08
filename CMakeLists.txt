cmake_minimum_required(VERSION 3.16)

project(BullsAndCows LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(bulls_and_cows main.cpp)

if (WIN32)
    target_compile_options(bulls_and_cows PRIVATE /utf-8)
    target_link_libraries(bulls_and_cows PRIVATE ws2_32)
endif()
