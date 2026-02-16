add_executable(heidi-kernel STATIC
    main.cpp
    status.cpp
)

target_link_libraries(heidi-kernel PRIVATE heidi-kernel-lib)

target_include_directories(heidi-kernel PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
)

target_compile_options(heidi-kernel PRIVATE -Wall -Wextra -Wpedantic)
