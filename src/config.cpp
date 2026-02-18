#include "heidi-kernel/config.h"

#include <cstdlib>
#include <string_view>

namespace heidi {

namespace {

constexpr std::string_view kVersion = "0.1.0";

} // namespace

Result<Config> ConfigParser::parse(int argc, char* argv[]) {
  Config config;
  config.log_level = "info";

  if (const char* env_sock = std::getenv("HEIDI_KERNEL_SOCK")) {
    config.socket_path = env_sock;
  }

  for (int i = 1; i < argc; ++i) {
    std::string_view arg = argv[i];

    if (arg == "--help" || arg == "-h") {
      config.show_help = true;
    } else if (arg == "--version" || arg == "-v") {
      config.show_version = true;
    } else if (arg == "--log-level" && i + 1 < argc) {
      config.log_level = argv[++i];
    } else if (arg == "--config" && i + 1 < argc) {
      config.config_path = argv[++i];
    } else if (arg == "--socket-path" && i + 1 < argc) {
      config.socket_path = argv[++i];
    } else if (!arg.starts_with("-")) {
      return Result<Config>::error(ErrorCode::InvalidArgument,
                                   std::string_view{"Unknown argument"});
    }
  }

  return Result<Config>::ok(config);
}

std::string_view ConfigParser::version() {
  return kVersion;
}

} // namespace heidi
