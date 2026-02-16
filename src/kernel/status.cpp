#include "heidi-kernel/status.h"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

namespace {

constexpr std::string_view kVersion = "0.1.0";

uint64_t get_rss_kb() {
  static FILE* f = nullptr;
  if (!f) {
    f = fopen("/proc/self/statm", "r");
  }

  if (!f) {
    return 0;
  }
  rewind(f);

  long dummy = 0;
  long rss = 0;
  if (fscanf(f, "%ld %ld", &dummy, &rss) != 2) {
    return 0;
  }
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

    struct timeval tv = {1, 0};
    int ready = select(server_fd_ + 1, &fds, nullptr, nullptr, &tv);
    if (ready < 0) {
      break;
    }
    if (ready == 0) {
      continue;
    }

    int client_fd = accept(server_fd_, nullptr, nullptr);
    if (client_fd < 0) {
      if (errno != EINTR) {
        break;
      }
      continue;
    }
    handle_client(client_fd);
    ::close(client_fd);
  }
}

void StatusSocket::handle_client(int client_fd) {
  char buf[128];
  ssize_t n = read(client_fd, buf, sizeof(buf) - 1);
  if (n <= 0) {
    return;
  }
  buf[n] = '\0';

  if (strncmp(buf, "STATUS\n", 8) == 0) {
    std::string response = format_status();
    write(client_fd, response.c_str(), response.size());
  }
}

std::string StatusSocket::format_status() const {
  auto uptime = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - status_.start_time)
                    .count();

  char buf[512];
  int len = snprintf(buf, sizeof(buf),
                     "{\"version\":\"%s\",\"pid\":%d,\"uptime_ms\":%lld,\"rss_kb\":%llu,"
                     "\"threads\":%d,\"queue_depth\":%d}\n",
                     std::string(status_.version).c_str(), status_.pid,
                     static_cast<long long>(uptime), static_cast<unsigned long long>(get_rss_kb()),
                     get_thread_count(), status_.queue_depth);
  return std::string(buf, len);
}

} // namespace heidi
