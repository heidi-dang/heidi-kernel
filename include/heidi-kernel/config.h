#pragma once

#include <cstdint>
#include <string_view>

#include "heidi-kernel/result.h"

namespace heidi {

struct Config {
    std::string_view config_path;
    std::string_view log_level;
    bool show_version = false;
    bool show_help = false;
};

struct ConfigParser {
    static Result<Config> parse(int argc, char* argv[]);
    static std::string_view version();
};

}
