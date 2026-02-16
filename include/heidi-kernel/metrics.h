#pragma once

#include <string>
#include <cstdint>
#include <vector>

namespace heidi {

struct CpuStats {
    uint64_t user = 0;
    uint64_t nice = 0;
    uint64_t system = 0;
    uint64_t idle = 0;
    uint64_t iowait = 0;
    uint64_t irq = 0;
    uint64_t softirq = 0;
};

struct MemStats {
    uint64_t total = 0;
    uint64_t free = 0;
    uint64_t available = 0;
    uint64_t buffers = 0;
    uint64_t cached = 0;
};

struct SystemMetrics {
    double cpu_usage_percent = 0.0;
    MemStats mem;
    uint64_t timestamp = 0;
};

class MetricsSampler {
public:
    MetricsSampler();
    SystemMetrics sample();
    
private:
    CpuStats prev_cpu_;
    bool first_sample_ = true;
    
    CpuStats read_cpu_stats();
    MemStats read_mem_stats();
};

} // namespace heidi