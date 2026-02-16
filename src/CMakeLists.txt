add_library(heidi-kernel-lib STATIC
    logger.cpp
    result.cpp
    event_loop.cpp
    config.cpp
    common/status.cpp
)

target_include_directories(heidi-kernel-lib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_options(heidi-kernel-lib PRIVATE -Wall -Wextra -Wpedantic)

# Kernel daemon
add_executable(heidi-kerneld
    daemon/main.cpp
)

target_link_libraries(heidi-kerneld heidi-kernel-lib)

# Dashboard daemon (HTTP bridge)
add_executable(dashd
    dashd/main.cpp
)

target_link_libraries(dashd heidi-kernel-lib)

# Control client
add_executable(hkctl
    tools/hkctl.cpp
)

target_link_libraries(hkctl heidi-kernel-lib)
