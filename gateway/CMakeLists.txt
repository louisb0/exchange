file(GLOB_RECURSE PROGRAM_SOURCES "src/*.cpp")
add_executable(gateway ${PROGRAM_SOURCES})
target_link_libraries(gateway common)

file(GLOB_RECURSE TEST_SOURCES "tests/test_*.cpp")
add_executable(gateway_tests ${TEST_SOURCES})
target_link_libraries(gateway_tests
    common gtest_main gtest
)

include(GoogleTest)
gtest_discover_tests(gateway_tests)
