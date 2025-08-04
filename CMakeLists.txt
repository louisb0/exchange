cmake_minimum_required(VERSION 3.20)
project(exchange)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(-Wall -Wextra -pedantic -fsanitize=address -fsanitize=undefined -g)
add_link_options(-fsanitize=address -fsanitize=undefined)

add_executable(engine engine/main.cpp)
target_include_directories(engine PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(engine engine/main.cpp)
target_include_directories(engine PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(gateway gateway/main.cpp)
target_include_directories(gateway PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
