#pragma once

#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>

#include "heidi-kernel/metrics.h"
#include "heidi-kernel/resource_governor.h"
#include "heidi-kernel/job.h"

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

    std::string update_policy(const std::string& json_body);

private:
    void sampling_thread();
    void monitor_loop();
    void handle_monitor_tick();
    
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
    ResourceGovernor* governor_;

    // Governor state
    mutable std::mutex governor_mutex_;
    int running_jobs_ = 0;
    int queued_jobs_ = 0;
    int rejected_jobs_ = 0;
    BlockReason blocked_reason_ = BlockReason::NONE;
    uint64_t retry_after_ms_ = 0;
    TickDiagnostics last_tick_diagnostics_;
    size_t jobs_started_this_tick_ = 0;
    size_t jobs_scanned_this_tick_ = 0;

    // Monitor timer
    int timer_fd_ = -1;
    std::thread monitor_thread_;
};

} // namespace heidi