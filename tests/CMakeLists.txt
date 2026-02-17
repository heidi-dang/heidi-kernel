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
    test_ipc.cpp
    test_metrics.cpp
    test_job.cpp
    test_governor.cpp
    test_policy_store.cpp
    test_http.cpp
    ../src/config.cpp
    ../src/dashd/http.cpp
)

target_include_directories(unit_tests PRIVATE ../include)
target_compile_options(unit_tests PRIVATE -std=c++23)

target_link_libraries(unit_tests
    PRIVATE
        gtest_main
        heidi-ipc
        heidi-metrics
        heidi-kernel-job
        heidi-kernel-governor
)

include(GoogleTest)
gtest_discover_tests(unit_tests)

add_subdirectory(integration)
