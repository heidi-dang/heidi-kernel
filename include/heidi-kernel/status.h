#pragma once

#include "heidi-kernel/event.h"
#include "heidi-kernel/ring_buffer.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

namespace heidi {

struct KernelStatus {
  uint32_t protocol_version;
  std::string_view version;
  int pid;
  std::chrono::steady_clock::time_point start_time;
  uint64_t rss_kb;
  int threads;
  int queue_depth;
};

class StatusSocket {
public:
  explicit StatusSocket(std::string_view socket_path);
  ~StatusSocket();

  void bind();
  void close();
  void set_stop();
  void serve_forever();
  KernelStatus& status() {
    return status_;
  }
  void publish_event(const Event& event);

private:
  void handle_client(int client_fd);
  std::string format_status() const;

  std::string socket_path_;
  int server_fd_ = -1;
  KernelStatus status_;
  std::atomic<bool> stop_requested_{false};
  RingBuffer<Event> ring_buffer_{100};
  std::vector<int> subscribers_;
  mutable std::mutex subscribers_mutex_;
};

} // namespace heidi
