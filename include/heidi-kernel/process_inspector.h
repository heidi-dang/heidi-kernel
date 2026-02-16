#pragma once

#include <unistd.h>

namespace heidi {

// Interface for inspecting process groups (injected for testability)
class IProcessInspector {
public:
    virtual ~IProcessInspector() = default;
    
    // Count processes in a process group
    // Returns: count of processes, or -1 on error
    // Bounded scan: implementation should cap iterations to avoid pathological load
    virtual int count_processes_in_pgid(pid_t pgid) = 0;
};

// Production Linux implementation using /proc
class ProcfsProcessInspector : public IProcessInspector {
public:
    // Hard cap on /proc entries to visit per call (avoid pathological load)
    static constexpr int kMaxProcEntries = 5000;
    
    int count_processes_in_pgid(pid_t pgid) override;
};

} // namespace heidi
