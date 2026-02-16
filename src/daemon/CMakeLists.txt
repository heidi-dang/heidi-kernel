add_executable(heidi-kernel-daemon main.cpp daemon.cpp)

target_include_directories(heidi-kernel-daemon
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../../include
)

target_link_libraries(heidi-kernel-daemon
    PRIVATE
        heidi-ipc
        heidi-metrics
        heidi-kernel-job
        heidi-kernel-lib
)

target_compile_options(heidi-kernel-daemon PRIVATE -Wall -Wextra -Wpedantic)