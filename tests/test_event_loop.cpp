#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <thread>

#include "heidi-kernel/event_loop.h"

TEST_CASE("EventLoop starts and stops", "[event_loop]") {
    heidi::EventLoop loop{std::chrono::milliseconds{10}};
    REQUIRE(loop.is_running() == false);

    loop.run();
    REQUIRE(loop.is_running() == true);

    loop.request_stop();

    std::this_thread::sleep_for(std::chrono::milliseconds{50});
    REQUIRE(loop.is_running() == false);
}

TEST_CASE("EventLoop tick callback is called", "[event_loop]") {
    heidi::EventLoop loop{std::chrono::milliseconds{20}};
    int tick_count = 0;

    loop.set_tick_callback([&tick_count](std::chrono::milliseconds) {
        ++tick_count;
    });

    loop.run();
    std::this_thread::sleep_for(std::chrono::milliseconds{100});
    loop.request_stop();

    while (loop.is_running()) {
        std::this_thread::sleep_for(std::chrono::milliseconds{5});
    }

    REQUIRE(tick_count > 0);
}
