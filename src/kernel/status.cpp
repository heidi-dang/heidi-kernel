#include "heidi-kernel/status.h"

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <iomanip>
#include <sstream>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

namespace {
constexpr std::string_view kVersion = "0.1.0";

uint64_t get_rss_kb() {
  FILE* f = fopen("/proc/self/statm", "r");
  if (!f) {
    return 0;
  }
  long rss = 0;
  if (fscanf(f, "%*ld %ld", &rss) != 1) {
    fclose(f);
    return 0;
  }
  fclose(f);
  return static_cast<uint64_t>(rss * 4096 / 1024);
}

int get_thread_count() {
  FILE* f = fopen("/proc/self/status", "r");
  if (!f) {
    return 1;
  }
  int threads = 1;
  char buf[256];
  while (fgets(buf, sizeof(buf), f)) {
    if (strncmp(buf, "Threads:", 8) == 0) {
      sscanf(buf + 8, "%d", &threads);
      break;
    }
  }
  fclose(f);
  return threads;
}
} // namespace

namespace heidi {

StatusSocket::StatusSocket(std::string_view socket_path) : socket_path_(socket_path) {
  status_.protocol_version = 1;
  status_.version = kVersion;
  status_.pid = getpid();
  status_.start_time = std::chrono::steady_clock::now();
  status_.queue_depth = 0;
}

StatusSocket::~StatusSocket() {
  close();
}

void StatusSocket::bind() {
  unlink(socket_path_.c_str());
  server_fd_ = socket(AF_UNIX, SOCK_STREAM, 0);
  if (server_fd_ < 0) {
    return;
  }

  struct sockaddr_un addr;
  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, socket_path_.c_str(), sizeof(addr.sun_path) - 1);

  if (::bind(server_fd_, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) {
    ::close(server_fd_);
    server_fd_ = -1;
    return;
  }
  chmod(socket_path_.c_str(), 0666);

  chmod(socket_path_.c_str(), 0666);

  if (listen(server_fd_, 5) < 0) {
    ::close(server_fd_);
    server_fd_ = -1;
  }
}

void StatusSocket::close() {
  stop_requested_ = true;
  if (server_fd_ >= 0) {
    ::close(server_fd_);
    server_fd_ = -1;
  }
  unlink(socket_path_.c_str());
}

void StatusSocket::set_stop() {
  stop_requested_ = true;
}

void StatusSocket::serve_forever() {
  if (server_fd_ < 0) {
    return;
  }

  while (!stop_requested_) {
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(server_fd_, &fds);
    int max_fd = server_fd_;

    {
      std::lock_guard<std::mutex> lock(subscribers_mutex_);
      for (int fd : subscribers_) {
        FD_SET(fd, &fds);
        if (fd > max_fd)
          max_fd = fd;
      }
    }

    struct timeval tv = {1, 0};
    int ready = select(max_fd + 1, &fds, nullptr, nullptr, &tv);
    if (ready < 0) {
      if (errno == EINTR)
        continue;
      break;
    }
    if (ready == 0) {
      continue;
    }

    if (FD_ISSET(server_fd_, &fds)) {
      int client_fd = accept(server_fd_, nullptr, nullptr);
      if (client_fd >= 0) {
        handle_client(client_fd);
      }
    }

    std::lock_guard<std::mutex> lock(subscribers_mutex_);
    for (auto it = subscribers_.begin(); it != subscribers_.end();) {
      if (FD_ISSET(*it, &fds)) {
        char dummy[128];
        ssize_t n = recv(*it, dummy, sizeof(dummy), MSG_DONTWAIT);
        if (n <= 0) {
          ::close(*it);
          it = subscribers_.erase(it);
          continue;
        }
      }
      ++it;
    }
  }
}

void StatusSocket::handle_client(int client_fd) {
  fd_set fds;
  FD_ZERO(&fds);
  FD_SET(client_fd, &fds);
  struct timeval tv = {0, 500000}; // 0.5s timeout
  int ready = select(client_fd + 1, &fds, nullptr, nullptr, &tv);
  if (ready <= 0) {
    ::close(client_fd);
    return;
  }

  char buf[128];
  ssize_t n = read(client_fd, buf, sizeof(buf) - 1);
  if (n <= 0) {
    ::close(client_fd);
    return;
  }
  buf[n] = '\0';

  std::string_view request(buf);
  while (!request.empty() && (request.back() == '\n' || request.back() == '\r')) {
    request.remove_suffix(1);
  }

  std::string response;
  if (request == "ping") {
    response = "PONG\n";
  } else if (request == "version") {
    char vbuf[128];
    snprintf(vbuf, sizeof(vbuf), "PROTOCOL %u DAEMON %s\n", status_.protocol_version,
             std::string(status_.version).c_str());
    response = vbuf;
  } else if (request == "status" || request == "status/json" || request == "STATUS") {
    response = format_status();
  } else if (request == "SUBSCRIBE") {
    std::lock_guard<std::mutex> lock(subscribers_mutex_);
    subscribers_.push_back(client_fd);
    auto events = ring_buffer_.last_n(100);
    for (const auto& event : events) {
      std::string msg = event.to_json() + "\n";
      send(client_fd, msg.c_str(), msg.size(), MSG_NOSIGNAL);
    }
  } else if (!request.empty()) {
    response = "ERR UNKNOWN_COMMAND " + std::string(request) + "\n";
  }

  if (!response.empty()) {
    write(client_fd, response.c_str(), response.size());
  }
}

std::string StatusSocket::format_status() const {
  auto uptime = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - status_.start_time)
                    .count();

  char buf[512];
  int len = snprintf(buf, sizeof(buf),
                     "{\"protocol_version\":%u,\"version\":\"%s\",\"pid\":%d,\"uptime_ms\":%lld,"
                     "\"rss_kb\":%llu,\"threads\":%d,\"queue_depth\":%d}\n",
                     status_.protocol_version, std::string(status_.version).c_str(), status_.pid,
                     static_cast<long long>(uptime), static_cast<unsigned long long>(get_rss_kb()),
                     get_thread_count(), status_.queue_depth);
  return std::string(buf, len);
}

void StatusSocket::publish_event(const Event& event) {
  ring_buffer_.push(event);
  std::string msg = event.to_json() + "\n";
  std::lock_guard<std::mutex> lock(subscribers_mutex_);
  for (auto it = subscribers_.begin(); it != subscribers_.end();) {
    if (send(*it, msg.c_str(), msg.size(), MSG_NOSIGNAL) < 0) {
      ::close(*it);
      it = subscribers_.erase(it);
    } else {
      ++it;
    }
  }
}

} // namespace heidi
