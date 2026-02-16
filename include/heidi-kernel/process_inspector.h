#pragma once

#include <cstdint>
#include <unistd.h>

namespace heidi {

struct IProcessInspector {
    virtual ~IProcessInspector() = default;
    virtual int count_processes_in_pgid(pid_t pgid) = 0;
};

struct ProcfsProcessInspector : IProcessInspector {
    int count_processes_in_pgid(pid_t pgid) override;
};

} // namespace heidi