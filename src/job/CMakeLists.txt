add_library(heidi-kernel-job STATIC
    job.cpp
    process_inspector_procfs.cpp
    procfs_starttime.cpp
)

target_include_directories(heidi-kernel-job
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../../include
)

target_link_libraries(heidi-kernel-job
    PUBLIC
        heidi-kernel-lib
)

target_compile_options(heidi-kernel-job PRIVATE -Wall -Wextra -Wpedantic)
