add_library(heidi-kernel-lib STATIC
    logger.cpp
    result.cpp
    event_loop.cpp
    config.cpp
)

target_include_directories(heidi-kernel-lib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_compile_options(heidi-kernel-lib PRIVATE -Wall -Wextra -Wpedantic)

add_subdirectory(ipc)
add_subdirectory(metrics)
add_subdirectory(job)
add_subdirectory(kernel)
add_subdirectory(dashd)
add_subdirectory(daemon)
