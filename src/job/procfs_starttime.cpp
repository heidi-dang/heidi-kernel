#include "procfs_starttime.h"

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>

namespace heidi {

std::optional<uint64_t> read_proc_start_time_ticks(pid_t pid) {
  char stat_path[256];
  snprintf(stat_path, sizeof(stat_path), "/proc/%d/stat", pid);
  FILE* stat_file = fopen(stat_path, "r");
  if (!stat_file)
    return std::nullopt;

  char buf[4096];
  std::optional<uint64_t> result = std::nullopt;
  if (!fgets(buf, sizeof(buf), stat_file)) {
    fclose(stat_file);
    return std::nullopt;
  }
  // Delegate to the parsing helper so tests can directly exercise it.
  result = parse_start_time_from_stat_line(buf);

  fclose(stat_file);
  return result;
}

std::optional<uint64_t> parse_start_time_from_stat_line(const char* stat_line) {
  if (!stat_line)
    return std::nullopt;
  // Find the last ')' which terminates comm.
  const char* p = strrchr(stat_line, ')');
  if (!p)
    return std::nullopt;

  int token_idx = 3; // field index of the next token after ')'
  const char* s = p + 1;
  // Defensive: if buffer is short, still attempt parsing; tokens are bounded.
  // The loop below will naturally exit if input ends early.
  while (*s) {
    // skip whitespace
    while (*s && isspace((unsigned char)*s))
      ++s;
    if (!*s)
      break;
    const char* t = s;
    while (*s && !isspace((unsigned char)*s))
      ++s;
    if (token_idx == 22) {
      // parse token [t, s)
      char tmp[64] = {0};
      int len = (int)(s - t);
      if (len > 0 && len < (int)sizeof(tmp)) {
        memcpy(tmp, t, len);
        tmp[len] = '\0';
        uint64_t v = strtoull(tmp, nullptr, 10);
        return v;
      }
      return std::nullopt;
    }
    token_idx++;
  }
  return std::nullopt;
}

} // namespace heidi
