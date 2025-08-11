file(GLOB_RECURSE PROGRAM_SOURCES "src/*.cpp")
add_executable(client ${PROGRAM_SOURCES})
target_link_libraries(client common)
