#pragma once

#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>

#include "heidi-kernel/metrics.h"

namespace heidi {

class MetricsHistory;
class JobRunner;

class Daemon {
public:
    Daemon(const std::string& socket_path, const std::string& state_dir = "/tmp/heidi-kernel-state");
    ~Daemon();

    void run();
    void stop();

    SystemMetrics get_latest_metrics() const;
    std::vector<SystemMetrics> get_metrics_tail(size_t n) const;

private:
    void sampling_thread();
    
    std::string socket_path_;
    std::string state_dir_;
    bool running_ = false;
    
    mutable std::mutex metrics_mutex_;
    SystemMetrics latest_metrics_;
    MetricsHistory* history_;
    
    std::thread sampler_thread_;
    std::condition_variable cv_;
    std::mutex cv_mutex_;

    JobRunner* job_runner_;
};

} // namespace heidi