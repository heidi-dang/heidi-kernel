add_library(heidi-kernel-lib STATIC
    logger.cpp
    event_loop.cpp
    config.cpp
)

target_include_directories(heidi-kernel-lib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_compile_options(heidi-kernel-lib PRIVATE -Wall -Wextra -Wpedantic)

add_subdirectory(kernel)
add_subdirectory(dashd)
