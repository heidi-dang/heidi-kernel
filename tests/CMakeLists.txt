include(FetchContent)

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(http_parser_test
    test_http_parser.cpp
    ../src/dashd/http.cpp
)

target_link_libraries(http_parser_test
    PRIVATE
    GTest::gtest_main
    pthread
    heidi-kernel-lib
)

target_include_directories(http_parser_test
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

include(GoogleTest)
gtest_discover_tests(http_parser_test)
