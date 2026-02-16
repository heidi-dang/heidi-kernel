#include "heidi-kernel/http.h"

#include <atomic>
#include <chrono>
#include <csignal>
#include <gtest/gtest.h>
#include <thread>

// Simple test to verify shutdown mechanism
TEST(HttpServerTest, ShutdownWorks) {
  volatile std::sig_atomic_t running = 1;
  heidi::HttpServer server("127.0.0.1", 7799);

  std::thread server_thread([&]() { server.serve_forever(&running); });

  // Give it a moment to start
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // Signal shutdown
  running = 0;

  // Join thread - if shutdown works, this should return quickly.
  // If it hangs, the test framework will kill it eventually (timeout).
  server_thread.join();

  SUCCEED();
}
