#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <atomic>
#include <cstdint>

namespace heidi {

// Forward declarations
class IProcessInspector;

enum class JobStatus {
    PENDING,
    RUNNING,
    COMPLETED,
    FAILED,
    CANCELLED,
    PROC_LIMIT  // New: terminated due to process count limit
};

struct Job {
    std::string id;
    std::string command;
    JobStatus status = JobStatus::PENDING;
    int exit_code = -1;
    std::string output;
    std::string error;
    
    // Time tracking (milliseconds since epoch)
    int64_t created_at_ms = 0;
    int64_t started_at_ms = 0;
    int64_t finished_at_ms = 0;
    
    // Process group ID for tracking
    pid_t pgid = -1;
    
    // For PROC_LIMIT enforcement
    int64_t last_scanned_at_ms = 0;
    bool kill_signal_sent = false;
    int64_t sigterm_sent_at_ms = 0;
};

// Resource limits policy
struct ResourcePolicy {
    int max_concurrent_jobs = 10;
    int max_processes_per_job = 100;  // PROC_LIMIT threshold
    int kill_grace_ms = 5000;         // Time between SIGTERM and SIGKILL
    
    // Tick budgets
    int max_job_starts_per_tick = 5;
    int max_job_scans_per_tick = 10;
};

// Tick diagnostics for observability
struct TickDiagnostics {
    int64_t tick_time_ms = 0;
    int jobs_started_this_tick = 0;
    int jobs_scanned_this_tick = 0;
    int total_running_jobs = 0;
    int scan_cursor_position = 0;
};

// Interface for process spawning/signaling (injected for testability)
class IProcessSpawner {
public:
    virtual ~IProcessSpawner() = default;
    
    // Spawn a process group, return PGID or -1 on error
    virtual pid_t spawn_process_group(const std::string& command) = 0;
    
    // Send signal to process group
    virtual bool signal_pgid(pid_t pgid, int signal) = 0;
    
    // Check if process group has completed, fill exit_code if so
    virtual bool check_completion(pid_t pgid, int& exit_code) = 0;
    
    // Collect output from process group (non-blocking)
    virtual void collect_output(pid_t pgid, std::string& stdout_out, std::string& stderr_out) = 0;
};

// Tick-driven JobRunner (deterministic, no sleeps, no detached threads)
class JobRunner {
public:
    // Production constructor
    JobRunner(const ResourcePolicy& policy = ResourcePolicy{});
    
    // Test constructor with injected dependencies
    JobRunner(const ResourcePolicy& policy,
              std::unique_ptr<IProcessSpawner> spawner,
              std::unique_ptr<IProcessInspector> inspector);
    
    ~JobRunner();

    // Main tick function - drive all progression through this
    // Returns: diagnostics for this tick
    TickDiagnostics tick(int64_t now_ms);

    // Submit a job (adds to pending queue)
    std::string submit_job(const std::string& command);
    
    // Cancel a pending or running job
    bool cancel_job(const std::string& job_id);
    
    // Get job status
    std::shared_ptr<Job> get_job_status(const std::string& job_id) const;
    
    // Get recent jobs
    std::vector<std::shared_ptr<Job>> get_recent_jobs(size_t limit = 10) const;
    
    // Backward compatibility: start/stop (no-ops in tick-driven model)
    void start() {}
    void stop() {}
    
    // Get current policy
    const ResourcePolicy& get_policy() const { return policy_; }
    
    // Update policy (returns false if invalid)
    bool update_policy(const ResourcePolicy& policy);

private:
    ResourcePolicy policy_;
    
    // Dependencies
    std::unique_ptr<IProcessSpawner> spawner_;
    std::unique_ptr<IProcessInspector> inspector_;
    
    // Job storage
    std::unordered_map<std::string, std::shared_ptr<Job>> jobs_;
    mutable std::mutex jobs_mutex_;
    
    // Tick state
    int scan_cursor_ = 0;  // Round-robin position for job scanning
    std::vector<std::string> job_ids_;  // Ordered list for round-robin
    
    // Generate unique job ID
    static std::string generate_job_id();
    
    // Tick sub-steps
    void start_pending_jobs(int64_t now_ms, TickDiagnostics& diag);
    void scan_running_jobs(int64_t now_ms, TickDiagnostics& diag);
    void check_job_completion(const std::shared_ptr<Job>& job, int64_t now_ms);
    void enforce_proc_limit(const std::shared_ptr<Job>& job, int64_t now_ms);
    void escalate_kill(const std::shared_ptr<Job>& job, int64_t now_ms);
};

} // namespace heidi
