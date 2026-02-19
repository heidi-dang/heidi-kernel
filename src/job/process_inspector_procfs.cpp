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
  int matched = 0;
  const int kMaxMatchedDump = 8;
  pid_t matched_pids[kMaxMatchedDump];
  int matched_pids_n = 0;
  int first_child_pid = 0;
  char leader_stat_raw[4096] = "";
  char first_child_stat_raw[4096] = "";

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
        matched++;
        // Collect matched pid list (bounded) for dump
        if (matched_pids_n < kMaxMatchedDump) {
          matched_pids[matched_pids_n++] = pid;
        }
        // Capture raw stat for leader (pid == pgid) and the first child we see
        if (pid == pgid) {
          strncpy(leader_stat_raw, buf, sizeof(leader_stat_raw) - 1);
          leader_stat_raw[sizeof(leader_stat_raw) - 1] = '\0';
        } else if (first_child_pid == 0) {
          first_child_pid = pid;
          strncpy(first_child_stat_raw, buf, sizeof(first_child_stat_raw) - 1);
          first_child_stat_raw[sizeof(first_child_stat_raw) - 1] = '\0';
        }
      }
    }

    fclose(stat_file);
  }

  closedir(proc_dir);
  // Minimal structured debug output for triage (local-only)
  if (matched >= 0) {
    // Trim newlines for nicer single-line output
    for (char* q = leader_stat_raw; *q; ++q)
      if (*q == '\n')
        *q = ' ';
    for (char* q = first_child_stat_raw; *q; ++q)
      if (*q == '\n')
        *q = ' ';

    // Build matched pid list (bounded)
    char matched_list[256] = "";
    int off = 0;
    for (int i = 0; i < matched_pids_n; ++i) {
      int n = snprintf(matched_list + off, sizeof(matched_list) - off, "%d%s", matched_pids[i],
                       (i + 1 < matched_pids_n) ? "," : "");
      if (n > 0)
        off += n;
      if (off >= (int)sizeof(matched_list) - 1)
        break;
    }
    if (matched_pids_n == 0)
      strncpy(matched_list, "<none>", sizeof(matched_list));

    fprintf(stderr,
            "PROC_CAP_INSPECTOR_DBG scan_pgid=%d scanned=%d matched=%d matched_pids=%s "
            "first_child_pid=%d "
            "leader_stat='%s' child_stat='%s'\n",
            pgid, scanned, matched, matched_list, first_child_pid,
            leader_stat_raw[0] ? leader_stat_raw : "<missing>",
            first_child_stat_raw[0] ? first_child_stat_raw : "<missing>");
  }

  return count;
}

} // namespace heidi
