#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <mutex>
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
  void tick_loop();

  std::chrono::milliseconds tick_interval_;
  std::atomic<bool> running_{false};
  std::atomic<bool> stop_requested_{false};
  TickCallback tick_callback_;
  std::thread worker_;
  std::mutex mutex_;
  std::condition_variable cv_;
};

} // namespace heidi
