#include "heidi-kernel/event_loop.h"

#include <chrono>
#include <condition_variable>
#include <thread>

namespace heidi {

EventLoop::EventLoop(std::chrono::milliseconds tick_interval)
    : tick_interval_(tick_interval) {}

EventLoop::~EventLoop() {
    request_stop();
}

void EventLoop::set_tick_callback(TickCallback cb) {
    tick_callback_ = std::move(cb);
}

void EventLoop::run() {
    if (running_.exchange(true)) {
        return;
    }

    worker_ = std::jthread([this](std::stop_token st) {
        tick_loop(st);
    });
}

void EventLoop::request_stop() {
    stop_requested_.store(true);
}

bool EventLoop::is_running() const noexcept {
    return running_.load();
}

std::chrono::milliseconds EventLoop::tick_interval() const noexcept {
    return tick_interval_;
}

void EventLoop::tick_loop(std::stop_token stop_token) {
    auto last_tick = std::chrono::steady_clock::now();

    while (!stop_token.stop_requested() && !stop_requested_.load()) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - last_tick);

        if (elapsed >= tick_interval_) {
            if (tick_callback_) {
                tick_callback_(elapsed);
            }
            last_tick = now;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds{10});
    }

    running_.store(false);
}

}
