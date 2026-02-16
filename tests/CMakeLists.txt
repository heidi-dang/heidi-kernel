include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/v1.14.0.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(http_test http_test.cpp)
target_link_libraries(http_test PRIVATE heidi-dashd-lib heidi-kernel-lib gtest_main pthread)
target_include_directories(http_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)
target_compile_options(http_test PRIVATE -Wall -Wextra -Wpedantic)

include(GoogleTest)
gtest_discover_tests(http_test)
