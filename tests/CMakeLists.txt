find_package(GTest REQUIRED)

add_executable(test_config test_config.cpp)

target_link_libraries(test_config
    PRIVATE
        heidi-kernel-lib
        GTest::gtest_main
)

add_test(NAME ConfigParserTest COMMAND test_config)
