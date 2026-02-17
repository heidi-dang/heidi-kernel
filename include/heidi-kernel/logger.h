#pragma once

#include <chrono>
#include <cstdint>
#include <iostream>
#include <mutex>
#include <ostream>
#include <string_view>

namespace heidi {

enum class LogLevel : uint8_t {
  Debug = 0,
  Info = 1,
  Warn = 2,
  Error = 3,
};

class Logger {
public:
  explicit Logger(std::ostream& out = std::cout, LogLevel min_level = LogLevel::Info);

  void set_level(LogLevel level);
  [[nodiscard]] LogLevel level() const;

  void debug(std::string_view msg);
  void info(std::string_view msg);
  void warn(std::string_view msg);
  void error(std::string_view msg);

  static std::string_view level_to_string(LogLevel level);

private:
  void log(LogLevel level, std::string_view msg);

  std::ostream& out_;
  LogLevel min_level_;
  std::mutex mutex_;
};

} // namespace heidi
