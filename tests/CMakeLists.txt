include(FetchContent)

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(unit_tests
    test_config.cpp
    ../src/config.cpp
)

target_include_directories(unit_tests PRIVATE ../include)
target_compile_options(unit_tests PRIVATE -std=c++23)

target_link_libraries(unit_tests
    PRIVATE
        gtest_main
)

add_executable(http_test
    http_test.cpp
)
target_link_libraries(http_test PRIVATE heidi-dashd-lib heidi-kernel-lib gtest_main)
target_include_directories(http_test PRIVATE ../include)
target_compile_options(http_test PRIVATE -std=c++23)

include(GoogleTest)
gtest_discover_tests(unit_tests)
gtest_discover_tests(http_test)
