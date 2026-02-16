cmake_minimum_required(VERSION 3.22)
project(heidi-kernel-tests)

add_executable(unit_tests
    main.cpp
    test_config.cpp
)

target_link_libraries(unit_tests PRIVATE heidi-kernel-lib)

add_test(NAME unit_tests COMMAND unit_tests)
