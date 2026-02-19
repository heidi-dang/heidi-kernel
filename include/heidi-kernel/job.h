#pragma once

#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <cstdint>
#include <queue>

#include "metrics.h"
#include "resource_governor.h"
#include "process_inspector.h"

namespace heidi {

struct JobLimits {
    uint64_t max_runtime_ms = 600000;
    uint64_t max_log_bytes = 10485760;
    uint64_t max_output_line_bytes = 65536;
    int max_child_processes = 64;
    uint64_t kill_grace_ms = 2000;
};

enum class JobStatus {
    QUEUED,
    STARTING,
    RUNNING,
    COMPLETED,
    FAILED,
    CANCELLED,
    TIMEOUT,
    PROC_LIMIT
};

struct Job {
    std::string id;
    std::string command;
    JobStatus status = JobStatus::QUEUED;
    int exit_code = -1;
    std::string output;
    std::string error;
    bool log_truncated = false;
    uint64_t bytes_written = 0;
    std::chrono::system_clock::time_point created_at;
    std::chrono::steady_clock::time_point started_at;
    std::chrono::system_clock::time_point finished_at;
    uint64_t started_at_ms = 0;
    uint64_t ended_at_ms = 0;
    pid_t process_group = -1;
    uint64_t max_runtime_ms = 600000; // 10 minutes default
    uint64_t max_log_bytes = 10485760; // 10MB default
    uint64_t max_output_line_bytes = 65536; // 64KB default
    int max_child_processes = 64;
    int stdout_fd = -1;
    int stderr_fd = -1;
};

struct TickDiagnostics {
    GovernorDecision last_decision = GovernorDecision::START_NOW;
    BlockReason last_block_reason = BlockReason::NONE;
    uint64_t last_retry_after_ms = 0;
    uint64_t last_tick_now_ms = 0;
    int last_tick_running = 0;
    int last_tick_queued = 0;
};

struct IProcessSpawner {
    virtual ~IProcessSpawner() = default;
    virtual bool spawn_job(Job& job, int* stdout_fd, int* stderr_fd) = 0;
};

class JobRunner {
public:
    JobRunner(size_t max_concurrent_jobs = 10, IProcessSpawner* spawner = nullptr, IProcessInspector* inspector = nullptr);
    ~JobRunner();

    void start();
    void stop();

    std::string submit_job(const std::string& command, const JobLimits& limits = JobLimits());
    bool cancel_job(const std::string& job_id);
    std::shared_ptr<Job> get_job_status(const std::string& job_id) const;
    std::vector<std::shared_ptr<Job>> get_recent_jobs(size_t limit = 10) const;

    // Diagnostic accessors
    const TickDiagnostics& get_last_tick_diagnostics() const { return last_tick_diagnostics_; }
    size_t get_jobs_started_this_tick() const { return jobs_started_this_tick_; }
    size_t get_jobs_scanned_this_tick() const { return jobs_scanned_this_tick_; }

    // Limit enforcement methods
    void tick(uint64_t now_ms, const SystemMetrics& metrics, size_t max_starts_per_tick = 5, size_t max_limit_scans_per_tick = 10);
    void check_job_limits(uint64_t now_ms, size_t max_jobs_to_check = 10);
    bool enforce_job_timeout(std::shared_ptr<Job> job, uint64_t now_ms);
    bool enforce_job_log_cap(std::shared_ptr<Job> job);
    bool enforce_job_process_cap(std::shared_ptr<Job> job, uint64_t now_ms);

private:
    void execute_job(std::shared_ptr<Job> job, uint64_t now_ms);

    size_t max_concurrent_;
    std::atomic<bool> running_{false};
    std::queue<std::shared_ptr<Job>> job_queue_;
    std::unordered_map<std::string, std::shared_ptr<Job>> jobs_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    TickDiagnostics last_tick_diagnostics_;
    size_t jobs_started_this_tick_ = 0;
    size_t jobs_scanned_this_tick_ = 0;
    size_t scan_cursor_ = 0;
    ResourceGovernor governor_;
    IProcessSpawner* spawner_;
    IProcessInspector* inspector_;
};

} // namespace heidi