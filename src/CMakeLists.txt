target_sources(heidi-kernel-lib
    PRIVATE
        logger.cpp
        result.cpp
        event_loop.cpp
        config.cpp
    PUBLIC
        FILE_SET headers
        BASE_HEADERS
        ${CMAKE_CURRENT_SOURCE_DIR}/../include/heidi-kernel/logger.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../include/heidi-kernel/result.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../include/heidi-kernel/event_loop.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../include/heidi-kernel/config.h
)

target_include_directories(heidi-kernel-lib
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        $<INSTALL_INTERFACE:include>
)

target_compile_options(heidi-kernel-lib PRIVATE -Wall -Wextra -Wpedantic)
