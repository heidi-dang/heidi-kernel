add_library(heidi-kernel-governor STATIC
    resource_governor.cpp
    gov_rule.cpp
    process_governor.cpp
    cgroup_driver.cpp
)

target_include_directories(heidi-kernel-governor
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../../include
)

target_link_libraries(heidi-kernel-governor
    PUBLIC
        heidi-kernel-lib
)

target_compile_options(heidi-kernel-governor PRIVATE -Wall -Wextra -Wpedantic)