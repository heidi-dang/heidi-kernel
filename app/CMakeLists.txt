target_sources(heidi-kernel PRIVATE main.cpp)

target_link_libraries(heidi-kernel PRIVATE heidi-kernel-lib)

target_compile_options(heidi-kernel PRIVATE -Wall -Wextra -Wpedantic)
