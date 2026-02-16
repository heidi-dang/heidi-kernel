add_executable(heidi-kernel main.cpp daemon.cpp)

target_link_libraries(heidi-kernel
    PRIVATE
        heidi-ipc
        heidi-kernel-lib
)

target_compile_options(heidi-kernel PRIVATE -Wall -Wextra -Wpedantic)