add_executable(heidi-dashd
    main.cpp
    http.cpp
)

target_link_libraries(heidi-dashd PRIVATE heidi-kernel-lib pthread)

target_include_directories(heidi-dashd PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
)

target_compile_options(heidi-dashd PRIVATE -Wall -Wextra -Wpedantic)
