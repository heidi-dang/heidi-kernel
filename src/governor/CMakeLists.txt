add_library(heidi-kernel-governor STATIC
    resource_governor.cpp
    gov_rule.cpp
    process_governor.cpp
<<<<<<< HEAD
    cgroup_driver.cpp
=======
>>>>>>> 33a8683 (feat(gov): initial process resource governor (P1-1 + P1-2 core))
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
