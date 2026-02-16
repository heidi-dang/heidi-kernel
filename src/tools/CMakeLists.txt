add_executable(heidi-kernelctl kernelctl.cpp)

target_compile_options(heidi-kernelctl PRIVATE -Wall -Wextra -Wpedantic)