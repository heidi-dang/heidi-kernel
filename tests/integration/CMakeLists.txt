include(FetchContent)

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

# Helper binaries
add_executable(hk_sleep ../helpers/hk_sleep.cpp)
target_include_directories(hk_sleep PRIVATE ../../include)
target_link_libraries(hk_sleep PRIVATE heidi-kernel-lib)

add_executable(hk_forkbomb_safe ../helpers/hk_forkbomb_safe.cpp)
target_include_directories(hk_forkbomb_safe PRIVATE ../../include)
target_link_libraries(hk_forkbomb_safe PRIVATE heidi-kernel-lib)

# Integration test executable
add_executable(integration_tests test_integration.cpp)

target_include_directories(integration_tests PRIVATE ../../include)
target_compile_options(integration_tests PRIVATE -std=c++23)

target_link_libraries(integration_tests
    PRIVATE
        gtest_main
        heidi-ipc
        heidi-metrics
        heidi-kernel-job
        heidi-kernel-governor
        heidi-kernel-core
)

include(GoogleTest)
gtest_discover_tests(integration_tests)