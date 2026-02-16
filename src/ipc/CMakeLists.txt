add_library(heidi-ipc STATIC ipc.cpp)

target_include_directories(heidi-ipc
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../../include
)

target_compile_options(heidi-ipc PRIVATE -Wall -Wextra -Wpedantic)