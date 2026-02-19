#include "heidi-kernel/event_loop.h"

#include <chrono>
#include <thread>

namespace heidi {

EventLoop::EventLoop(std::chrono::milliseconds tick_interval)
    : tick_interval_(tick_interval) {}

EventLoop::~EventLoop() {
    request_stop();
    if (worker_.joinable()) {
        worker_.join();
    }
}

void EventLoop::set_tick_callback(TickCallback cb) {
    tick_callback_ = std::move(cb);
}

void EventLoop::run() {
    if (running_.exchange(true)) {
        return;
    }

    worker_ = std::thread([this]() {
        tick_loop();
    });
}

void EventLoop::request_stop() {
    stop_requested_.store(true);
    cv_.notify_one();
}

bool EventLoop::is_running() const noexcept {
    return running_.load();
}

std::chrono::milliseconds EventLoop::tick_interval() const noexcept {
    return tick_interval_;
}

void EventLoop::tick_loop() {
    auto last_tick = std::chrono::steady_clock::now();

    while (!stop_requested_.load()) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - last_tick);

        if (elapsed >= tick_interval_) {
            if (tick_callback_) {
                tick_callback_(elapsed);
            }
            last_tick = now;
        }

        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait_for(lock, std::chrono::milliseconds{10}, [this] {
            return stop_requested_.load();
        });
    }

    running_.store(false);
}

}
