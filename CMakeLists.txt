cmake_minimum_required(VERSION 3.20)
project(exchange)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(-Wall -Wextra -pedantic)

add_executable(gateway gateway/main.cpp)
target_include_directories(gateway PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
