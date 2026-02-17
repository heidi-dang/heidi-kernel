#include "heidi-kernel/logger.h"

#include <gtest/gtest.h>
#include <sstream>
#include <thread>
#include <vector>

TEST(LoggerTest, ThreadSafety) {
  std::stringstream ss;
  heidi::Logger logger(ss);

  const int num_threads = 10;
  const int num_logs = 100;
  std::vector<std::thread> threads;

  for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back([&logger, i]() {
      for (int j = 0; j < num_logs; ++j) {
        logger.info("Thread " + std::to_string(i) + " log " + std::to_string(j));
      }
    });
  }

  for (auto& t : threads) {
    t.join();
  }

  // Basic verification: count newlines
  std::string output = ss.str();
  int newlines = 0;
  for (char c : output) {
    if (c == '\n') {
      newlines++;
    }
  }

  EXPECT_EQ(newlines, num_threads * num_logs);
}
