#include "heidi-kernel/process_inspector.h"

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace heidi {

int ProcfsProcessInspector::count_processes_in_pgid(pid_t pgid) {
  DIR* proc_dir = opendir("/proc");
  if (!proc_dir) {
    return -1; // Unable to access /proc
  }

  int count = 0;
  const int max_scans = 5000; // Safety cap to avoid pathological cost
  int scanned = 0;

  struct dirent* entry;
  while ((entry = readdir(proc_dir)) != nullptr && scanned < max_scans) {
    if (entry->d_type != DT_DIR)
      continue;

    // Check if entry is a pid directory (digits only)
    char* endptr;
    pid_t pid = strtol(entry->d_name, &endptr, 10);
    if (*endptr != '\0' || pid <= 0)
      continue;

    scanned++;

    // Read /proc/<pid>/stat
    char stat_path[256];
    snprintf(stat_path, sizeof(stat_path), "/proc/%d/stat", pid);

    FILE* stat_file = fopen(stat_path, "r");
    if (!stat_file)
      continue;

    // Format: pid (comm) state ppid pgrp session tty_nr tpgid flags minflt cminflt majflt cmajflt
    // utime stime cutime cstime priority nice num_threads itrealvalue starttime vsize rss rsslim
    // startcode endcode startstack kstkesp kstkeip signal blocked sigignore sigcatch wchan nswap
    // cnswap exit_signal processor rt_priority policy delayacct_blkio_ticks guest_time cguest_time
    // start_data end_data start_brk arg_start arg_end env_start env_end exit_code We need pgrp
    // (field 5, 1-indexed)
    // Read the full stat line to avoid broken parsing when comm contains spaces.
    // We'll parse by finding the closing ')' then scanning following fields.
    char buf[4096];
    if (!fgets(buf, sizeof(buf), stat_file)) {
      fclose(stat_file);
      continue;
    }
    // Find the ')' that ends the comm field
    char* p = strrchr(buf, ')');
    if (!p) {
      // malformed, skip
      fclose(stat_file);
      continue;
    }
    // Now parse fields after ')' - we expect: " state ppid pgrp ..."
    int ppid = 0;
    int pgrp = 0;
    if (sscanf(p + 1, " %*c %d %d", &ppid, &pgrp) == 2) {
      if (static_cast<pid_t>(pgrp) == pgid) {
        count++;
      }
    }

    fclose(stat_file);
  }

  closedir(proc_dir);
  return count;
}

} // namespace heidi
