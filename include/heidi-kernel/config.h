#pragma once

#include "heidi-kernel/result.h"

#include <cstdint>
#include <string_view>

namespace heidi {

struct Config {
  std::string_view config_path;
  std::string_view log_level;
  std::string_view socket_path;
  bool show_version = false;
  bool show_help = false;
};

struct ConfigParser {
  static Result<Config> parse(int argc, char* argv[]);
  static std::string_view version();
};

} // namespace heidi
