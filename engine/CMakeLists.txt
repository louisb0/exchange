file(GLOB_RECURSE PROGRAM_SOURCES "src/*.cpp")
add_executable(engine ${PROGRAM_SOURCES})
target_link_libraries(engine common)

file(GLOB_RECURSE TEST_SOURCES "tests/test_*.cpp")
add_executable(engine_tests ${TEST_SOURCES})
target_link_libraries(engine_tests
    common gtest_main gtest
)

include(GoogleTest)
gtest_discover_tests(engine_tests)
