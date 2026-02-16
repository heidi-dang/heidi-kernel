cmake_minimum_required(VERSION 3.22)
project(heidi-kernel VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(ENABLE_TESTS "Build tests" ON)

include(GNUInstallDirs)

add_subdirectory(src)

if(ENABLE_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
