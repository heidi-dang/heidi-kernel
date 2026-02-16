include(FetchContent)

FetchContent_Declare(
    catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.7.1
)

set(CATCH_DEVELOPMENT_BUILD ON)
FetchContent_MakeAvailable(catch2)

add_executable(heidi-kernel-tests
    test_logger.cpp
    test_result.cpp
    test_event_loop.cpp
)

target_link_libraries(heidi-kernel-tests PRIVATE Catch2::Catch2WithMain heidi-kernel-lib)

target_compile_options(heidi-kernel-tests PRIVATE -Wall -Wextra -Wpedantic)

add_test(NAME heidi-kernel-tests COMMAND heidi-kernel-tests)
