#include "heidi-kernel/config.h"

#include <string_view>

namespace heidi {

namespace {

constexpr std::string_view kVersion = "0.1.0";

} // namespace

Result<Config> ConfigParser::parse(int argc, char* argv[]) {
    Config config;
    config.log_level = "info";

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
<<<<<<< HEAD
        } else {
=======
        } else if (!arg.starts_with("-")) {
            return Result<Config>::error(
                ErrorCode::InvalidArgument,
                std::string_view{"Unknown argument"});
        } else {
            return Result<Config>::error(
                ErrorCode::InvalidArgument,
                std::string_view{"Unknown flag"});
        }
    }

    return Result<Config>::ok(config);
}

std::string_view ConfigParser::version() {
    return kVersion;
}

}
