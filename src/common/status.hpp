#pragma once

#include <string>
#include <chrono>
#include <cstdint>

namespace heidi {

struct Status {
    bool ok = true;
    int pid = 0;
    uint64_t uptime_ms = 0;
    std::string version;
    std::string sock_path;
    std::string build_git;
    
    // Serialize to JSON (hand-rolled, no external deps)
    std::string to_json() const;
    
    // Create error response
    static Status error(const std::string& msg);
};

} // namespace heidi
