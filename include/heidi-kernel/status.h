#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <string_view>

namespace heidi {

struct KernelStatus {
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

private:
  void handle_client(int client_fd);
  std::string format_status() const;

  std::string socket_path_;
  int server_fd_ = -1;
  KernelStatus status_;
  std::atomic<bool> stop_requested_{false};
};

} // namespace heidi
