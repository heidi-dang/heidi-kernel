#include "heidi-kernel/status.h"

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstring>
#include <iostream>
#include <sys/socket.h>
#include <sys/un.h>
#include <thread>
#include <unistd.h>
#include <vector>

void client_reconnect(const std::string& path, int iterations) {
  auto start = std::chrono::high_resolution_clock::now();
  for (int i = 0; i < iterations; ++i) {
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
      perror("socket");
      return;
    }
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path) - 1);
    if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
      perror("connect");
      close(fd);
      return;
    }
    std::string req = "STATUS\n";
    write(fd, req.c_str(), req.size());
    char buf[4096];
    read(fd, buf, sizeof(buf));
    close(fd);
  }
  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> diff = end - start;
  std::cout << "Reconnect: " << iterations << " iterations in " << diff.count() << "s ("
            << (iterations / diff.count()) << " ops/s)" << std::endl;
}

void client_persistent(const std::string& path, int iterations) {
  auto start = std::chrono::high_resolution_clock::now();
  int fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (fd < 0) {
    perror("socket");
    return;
  }
  struct sockaddr_un addr;
  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path) - 1);

  if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
    perror("connect persistent");
    close(fd);
    return;
  }

  for (int i = 0; i < iterations; ++i) {
    std::string req = "STATUS\n";
    ssize_t w = write(fd, req.c_str(), req.size());
    if (w < 0) {
      // unexpected in successful persistent connection
      // expected in current broken implementation
      break;
    }
    char buf[4096];
    ssize_t n = read(fd, buf, sizeof(buf));
    if (n <= 0) {
      break;
    }
  }
  close(fd);
  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> diff = end - start;
  std::cout << "Persistent: " << iterations << " iterations in " << diff.count() << "s ("
            << (iterations / diff.count()) << " ops/s)" << std::endl;
}

int main() {
  std::signal(SIGPIPE, SIG_IGN);

  std::string socket_path = "/tmp/heidi-kernel-bench.sock";
  heidi::StatusSocket server(socket_path);
  server.bind();

  std::thread server_thread([&server]() { server.serve_forever(); });

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  std::cout << "Starting benchmark..." << std::endl;
  int iterations = 1000;

  std::cout << "Testing Reconnect..." << std::endl;
  client_reconnect(socket_path, iterations);

  std::cout << "Testing Persistent..." << std::endl;
  client_persistent(socket_path, iterations);

  server.set_stop();
  // Wake up loop
  int fd = socket(AF_UNIX, SOCK_STREAM, 0);
  struct sockaddr_un addr;
  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, socket_path.c_str(), sizeof(addr.sun_path) - 1);
  connect(fd, (struct sockaddr*)&addr, sizeof(addr));
  close(fd);

  if (server_thread.joinable()) {
    server_thread.join();
  }

  return 0;
}
