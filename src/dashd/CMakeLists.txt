# Library for HTTP server (shared between daemon and tests)
add_library(heidi-dashd-lib STATIC
    http.cpp
)

target_link_libraries(heidi-dashd-lib PRIVATE heidi-kernel-lib pthread)

target_include_directories(heidi-dashd-lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
)

target_compile_options(heidi-dashd-lib PRIVATE -Wall -Wextra -Wpedantic)

# Daemon executable
add_executable(heidi-dashd
    main.cpp
)

target_link_libraries(heidi-dashd PRIVATE heidi-dashd-lib heidi-kernel-lib pthread)

target_include_directories(heidi-dashd PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
)

target_compile_options(heidi-dashd PRIVATE -Wall -Wextra -Wpedantic)
