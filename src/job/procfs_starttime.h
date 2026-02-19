// Small helper to read the start_time (clock ticks since boot) from /proc/<pid>/stat
#pragma once

#include <cstdint>
#include <optional>
#include <sys/types.h>

namespace heidi {

// Return start_time (field 22 in /proc/<pid>/stat) as ticks, or std::nullopt on
// parse/read failure.
std::optional<uint64_t> read_proc_start_time_ticks(pid_t pid);

// Parse a full /proc/<pid>/stat line and return start_time (field 22) if
// present. The caller should ensure the buffer contains a NUL-terminated line
// as read from procfs. Exposed for unit testing.
std::optional<uint64_t> parse_start_time_from_stat_line(const char* stat_line);

} // namespace heidi
