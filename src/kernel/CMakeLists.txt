add_library(heidi-kernel-core STATIC
    main.cpp
    status.cpp
)

target_link_libraries(heidi-kernel-core PRIVATE heidi-kernel-lib pthread)

target_include_directories(heidi-kernel-core PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
)

target_compile_options(heidi-kernel-core PRIVATE -Wall -Wextra -Wpedantic)

add_executable(heidi-kernel)
target_sources(heidi-kernel PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../app/main.cpp)
target_link_libraries(heidi-kernel PRIVATE heidi-kernel-core heidi-kernel-lib)
