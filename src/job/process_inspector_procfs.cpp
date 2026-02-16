#include "heidi-kernel/process_inspector.h"

#include <cstdio>
#include <dirent.h>
#include <cstring>
#include <string>

namespace heidi {

int ProcfsProcessInspector::count_processes_in_pgid(pid_t pgid) {
    if (pgid <= 0) {
        return 0;
    }
    
    DIR* proc_dir = opendir("/proc");
    if (!proc_dir) {
        return -1; // /proc not available
    }
    
    int count = 0;
    int entries_scanned = 0;
    struct dirent* entry;
    
    while ((entry = readdir(proc_dir)) != nullptr && entries_scanned < kMaxProcEntries) {
        entries_scanned++;
        
        // Check if entry name is numeric (PID)
        char* endptr;
        long pid = strtol(entry->d_name, &endptr, 10);
        if (*endptr != '\0') {
            continue; // Not a PID directory
        }
        
        // Read process group from /proc/<pid>/stat
        // Format: pid (comm) state ppid pgrp session ...
        std::string stat_path = "/proc/" + std::string(entry->d_name) + "/stat";
        FILE* stat_file = fopen(stat_path.c_str(), "r");
        if (!stat_file) {
            continue; // Process may have exited
        }
        
        // Parse stat file: need field 5 (pgrp)
        // The comm field can contain spaces and parentheses, so we need to be careful
        char line[1024];
        if (fgets(line, sizeof(line), stat_file)) {
            // Find the last ')' to skip the comm field
            char* paren_end = strrchr(line, ')');
            if (paren_end) {
                // After the ')', there's a space and then state, ppid, pgrp
                // Format: ... ) state ppid pgrp ...
                char state_char;
                pid_t ppid, pgrp;
                if (sscanf(paren_end + 1, " %c %d %d", &state_char, &ppid, &pgrp) >= 3) {
                    if (pgrp == pgid) {
                        count++;
                    }
                }
            }
        }
        fclose(stat_file);
    }
    
    closedir(proc_dir);
    return count;
}

} // namespace heidi
