add_library(heidi-metrics STATIC sampler.cpp)

target_include_directories(heidi-metrics
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../../include
)

target_compile_options(heidi-metrics PRIVATE -Wall -Wextra -Wpedantic)