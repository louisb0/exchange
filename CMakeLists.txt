cmake_minimum_required(VERSION 3.20)
project(exchange)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS_DEBUG "-g -Og -fno-omit-frame-pointer")

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.zip
)
FetchContent_Declare(
  spdlog
  URL https://github.com/gabime/spdlog/archive/refs/tags/v1.15.3.zip
)

FetchContent_MakeAvailable(googletest)
FetchContent_MakeAvailable(spdlog)

add_library(common INTERFACE)
target_include_directories(common INTERFACE common/)
target_link_libraries(common INTERFACE spdlog::spdlog)

enable_testing()

add_subdirectory(gateway)
# add_subdirectory(engine)
# add_subdirectory(client)
