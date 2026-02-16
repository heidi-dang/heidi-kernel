#pragma once

#include <atomic>
#include <chrono>
#include <functional>
#include <stop_token>
#include <thread>

namespace heidi {

class EventLoop {
public:
    using TickCallback = std::function<void(std::chrono::milliseconds tick)>;

    explicit EventLoop(std::chrono::milliseconds tick_interval = std::chrono::milliseconds{100});

    ~EventLoop();

    void set_tick_callback(TickCallback cb);
    void run();
    void request_stop();

    [[nodiscard]] bool is_running() const noexcept;
    [[nodiscard]] std::chrono::milliseconds tick_interval() const noexcept;

private:
    void tick_loop(std::stop_token stop_token);

    std::chrono::milliseconds tick_interval_;
    std::atomic<bool> running_{false};
    std::atomic<bool> stop_requested_{false};
    TickCallback tick_callback_;
    std::jthread worker_;
};

}
