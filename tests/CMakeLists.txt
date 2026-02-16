include(FetchContent)

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
# Prevent overriding the parent project's compiler/linker settings on Windows
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(unit_tests
    test_config.cpp
)

target_link_libraries(unit_tests
    PRIVATE
        heidi-kernel-lib
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(unit_tests)
