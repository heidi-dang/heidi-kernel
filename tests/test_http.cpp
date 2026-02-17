#include "heidi-kernel/http.h"

#include <atomic>
#include <chrono>
#include <gtest/gtest.h>
#include <thread>

TEST(HttpServerTest, StopGracefully) {
  // Use port 0 to let the OS assign a free port
  heidi::HttpServer server("127.0.0.1", 0);

  std::atomic<bool> stop_flag{false};

  // Start server in a separate thread
  std::thread server_thread([&]() { server.serve_forever([&]() { return stop_flag.load(); }); });

  // Let it run for a bit
  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  // Signal stop
  stop_flag.store(true);

  // Wait for thread to join
  // If serve_forever doesn't respect the stop flag, this join will hang (eventually timing out the
  // test)
  server_thread.join();

  SUCCEED();
}
