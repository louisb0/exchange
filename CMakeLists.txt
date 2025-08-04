cmake_minimum_required(VERSION 3.20)
project(exchange)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.zip
)
FetchContent_MakeAvailable(googletest)

add_library(common INTERFACE)
target_include_directories(common INTERFACE common/)

enable_testing()

add_subdirectory(gateway)
add_subdirectory(engine)
add_subdirectory(client)
