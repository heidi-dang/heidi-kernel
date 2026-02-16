include(FetchContent)

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(logger_test test_logger.cpp)
target_link_libraries(logger_test PRIVATE heidi-kernel-lib GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(logger_test)
