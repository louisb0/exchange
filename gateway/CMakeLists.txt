add_executable(gateway src/main.cpp)
target_link_libraries(gateway
    common
)

file(GLOB_RECURSE TEST_SOURCES "tests/test_*.cpp")
add_executable(gateway_tests ${TEST_SOURCES})
target_link_libraries(gateway_tests
    common gtest_main gtest
)

include(GoogleTest)
gtest_discover_tests(gateway_tests)
