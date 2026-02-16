#include "heidi-kernel/job.h"
#include "heidi-kernel/process_inspector.h"

#include <sstream>
#include <iomanip>
#include <algorithm>
#include <mutex>
#include <cstring>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>

namespace heidi {

// Production ProcessSpawner implementation
class PosixProcessSpawner : public IProcessSpawner {
public:
    struct ProcessInfo {
        pid_t pgid;
        int stdout_fd;
        int stderr_fd;
        std::string stdout_buffer;
        std::string stderr_buffer;
    };
    
    std::unordered_map<pid_t, ProcessInfo> processes;
    mutable std::mutex mutex;
    
    pid_t spawn_process_group(const std::string& command) override {
        int pipe_stdout[2];
        int pipe_stderr[2];
        
        if (pipe(pipe_stdout) == -1 || pipe(pipe_stderr) == -1) {
            return -1;
        }
        
        pid_t pid = fork();
        if (pid == -1) {
            close(pipe_stdout[0]); close(pipe_stdout[1]);
            close(pipe_stderr[0]); close(pipe_stderr[1]);
            return -1;
        }
        
        if (pid == 0) { // Child
            close(pipe_stdout[0]);
            close(pipe_stderr[0]);
            
            dup2(pipe_stdout[1], STDOUT_FILENO);
            dup2(pipe_stderr[1], STDERR_FILENO);
            close(pipe_stdout[1]);
            close(pipe_stderr[1]);
            
            // Create new process group
            setpgid(0, 0);
            
            execl("/bin/sh", "sh", "-c", command.c_str(), nullptr);
            _exit(1);
        } else { // Parent
            close(pipe_stdout[1]);
            close(pipe_stderr[1]);
            
            setpgid(pid, pid); // Set process group
            
            std::lock_guard<std::mutex> lock(mutex);
            processes[pid] = {pid, pipe_stdout[0], pipe_stderr[0], "", ""};
            return pid;
        }
    }
    
    bool signal_pgid(pid_t pgid, int signal) override {
        return kill(-pgid, signal) == 0 || kill(pgid, signal) == 0;
    }
    
    bool check_completion(pid_t pgid, int& exit_code) override {
        int status;
        pid_t result = waitpid(pgid, &status, WNOHANG);
        
        if (result == 0) {
            return false; // Still running
        } else if (result == pgid) {
            exit_code = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
            
            // Clean up process info
            std::lock_guard<std::mutex> lock(mutex);
            auto it = processes.find(pgid);
            if (it != processes.end()) {
                close(it->second.stdout_fd);
                close(it->second.stderr_fd);
                processes.erase(it);
            }
            return true;
        }
        return false;
    }
    
    void collect_output(pid_t pgid, std::string& stdout_out, std::string& stderr_out) override {
        std::lock_guard<std::mutex> lock(mutex);
        auto it = processes.find(pgid);
        if (it == processes.end()) return;
        
        char buffer[4096];
        ssize_t n;
        
        // Non-blocking read from stdout
        while ((n = read(it->second.stdout_fd, buffer, sizeof(buffer))) > 0) {
            it->second.stdout_buffer.append(buffer, n);
        }
        
        // Non-blocking read from stderr
        while ((n = read(it->second.stderr_fd, buffer, sizeof(buffer))) > 0) {
            it->second.stderr_buffer.append(buffer, n);
        }
        
        stdout_out = it->second.stdout_buffer;
        stderr_out = it->second.stderr_buffer;
    }
};

static std::string generate_job_id_impl() {
    static std::atomic<uint64_t> counter{0};
    uint64_t id = counter++;
    
    std::ostringstream oss;
    oss << "job_" << std::hex << id;
    return oss.str();
}

std::string JobRunner::generate_job_id() {
    return generate_job_id_impl();
}

// Production constructor
JobRunner::JobRunner(const ResourcePolicy& policy)
    : policy_(policy),
      spawner_(std::make_unique<PosixProcessSpawner>()),
      inspector_(std::make_unique<ProcfsProcessInspector>()) {
}

// Test constructor
JobRunner::JobRunner(const ResourcePolicy& policy,
                     std::unique_ptr<IProcessSpawner> spawner,
                     std::unique_ptr<IProcessInspector> inspector)
    : policy_(policy),
      spawner_(std::move(spawner)),
      inspector_(std::move(inspector)) {
}

JobRunner::~JobRunner() = default;

std::string JobRunner::submit_job(const std::string& command) {
    auto job = std::make_shared<Job>();
    job->id = generate_job_id();
    job->command = command;
    
    {
        std::lock_guard<std::mutex> lock(jobs_mutex_);
        jobs_[job->id] = job;
        job_ids_.push_back(job->id);
    }
    
    return job->id;
}

bool JobRunner::cancel_job(const std::string& job_id) {
    std::lock_guard<std::mutex> lock(jobs_mutex_);
    auto it = jobs_.find(job_id);
    if (it == jobs_.end()) {
        return false;
    }
    
    auto job = it->second;
    if (job->status == JobStatus::PENDING) {
        job->status = JobStatus::CANCELLED;
        job->finished_at_ms = 0; // Will be set by tick
        return true;
    } else if (job->status == JobStatus::RUNNING && job->pgid > 0) {
        // Send SIGTERM
        spawner_->signal_pgid(job->pgid, SIGTERM);
        job->kill_signal_sent = true;
        job->sigterm_sent_at_ms = 0; // Will be set by tick
        return true;
    }
    
    return false;
}

std::shared_ptr<Job> JobRunner::get_job_status(const std::string& job_id) const {
    std::lock_guard<std::mutex> lock(jobs_mutex_);
    auto it = jobs_.find(job_id);
    if (it == jobs_.end()) {
        return nullptr;
    }
    return it->second;
}

std::vector<std::shared_ptr<Job>> JobRunner::get_recent_jobs(size_t limit) const {
    std::lock_guard<std::mutex> lock(jobs_mutex_);
    std::vector<std::shared_ptr<Job>> result;
    
    for (const auto& pair : jobs_) {
        result.push_back(pair.second);
    }
    
    std::sort(result.begin(), result.end(),
              [](const auto& a, const auto& b) {
                  return a->created_at_ms > b->created_at_ms;
              });
    
    if (result.size() > limit) {
        result.resize(limit);
    }
    
    return result;
}

bool JobRunner::update_policy(const ResourcePolicy& policy) {
    // Basic validation
    if (policy.max_concurrent_jobs < 1 ||
        policy.max_processes_per_job < 1 ||
        policy.kill_grace_ms < 0 ||
        policy.max_job_starts_per_tick < 1 ||
        policy.max_job_scans_per_tick < 1) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(jobs_mutex_);
    policy_ = policy;
    return true;
}

TickDiagnostics JobRunner::tick(int64_t now_ms) {
    TickDiagnostics diag;
    diag.tick_time_ms = now_ms;
    
    start_pending_jobs(now_ms, diag);
    scan_running_jobs(now_ms, diag);
    
    std::lock_guard<std::mutex> lock(jobs_mutex_);
    diag.total_running_jobs = std::count_if(jobs_.begin(), jobs_.end(),
        [](const auto& pair) {
            return pair.second->status == JobStatus::RUNNING;
        });
    diag.scan_cursor_position = scan_cursor_;
    
    return diag;
}

void JobRunner::start_pending_jobs(int64_t now_ms, TickDiagnostics& diag) {
    std::lock_guard<std::mutex> lock(jobs_mutex_);
    
    // Count currently running jobs
    int running_count = std::count_if(jobs_.begin(), jobs_.end(),
        [](const auto& pair) {
            return pair.second->status == JobStatus::RUNNING;
        });
    
    int started = 0;
    for (const auto& job_id : job_ids_) {
        if (started >= policy_.max_job_starts_per_tick) {
            break;
        }
        if (running_count >= policy_.max_concurrent_jobs) {
            break;
        }
        
        auto it = jobs_.find(job_id);
        if (it == jobs_.end()) continue;
        
        auto job = it->second;
        if (job->status != JobStatus::PENDING) continue;
        
        // Start the job
        pid_t pgid = spawner_->spawn_process_group(job->command);
        if (pgid > 0) {
            job->status = JobStatus::RUNNING;
            job->pgid = pgid;
            job->started_at_ms = now_ms;
            running_count++;
            started++;
        } else {
            job->status = JobStatus::FAILED;
            job->finished_at_ms = now_ms;
        }
    }
    
    diag.jobs_started_this_tick = started;
}

void JobRunner::scan_running_jobs(int64_t now_ms, TickDiagnostics& diag) {
    std::lock_guard<std::mutex> lock(jobs_mutex_);
    
    if (job_ids_.empty()) return;
    
    int scanned = 0;
    size_t start_cursor = scan_cursor_;
    
    do {
        if (scanned >= policy_.max_job_scans_per_tick) {
            break;
        }
        
        const std::string& job_id = job_ids_[scan_cursor_];
        auto it = jobs_.find(job_id);
        
        scan_cursor_ = (scan_cursor_ + 1) % job_ids_.size();
        
        if (it == jobs_.end()) continue;
        
        auto job = it->second;
        if (job->status != JobStatus::RUNNING) continue;
        
        scanned++;
        job->last_scanned_at_ms = now_ms;
        
        // Check for completion
        check_job_completion(job, now_ms);
        
        // Enforce PROC_LIMIT if still running
        if (job->status == JobStatus::RUNNING) {
            enforce_proc_limit(job, now_ms);
        }
        
        // Check for kill escalation (SIGTERM -> SIGKILL)
        if (job->kill_signal_sent && job->status == JobStatus::RUNNING) {
            escalate_kill(job, now_ms);
        }
        
    } while (scan_cursor_ != start_cursor);
    
    diag.jobs_scanned_this_tick = scanned;
}

void JobRunner::check_job_completion(const std::shared_ptr<Job>& job, int64_t now_ms) {
    int exit_code = -1;
    if (spawner_->check_completion(job->pgid, exit_code)) {
        // Job completed
        spawner_->collect_output(job->pgid, job->output, job->error);
        job->exit_code = exit_code;
        job->status = (exit_code == 0) ? JobStatus::COMPLETED : JobStatus::FAILED;
        job->finished_at_ms = now_ms;
    }
}

void JobRunner::enforce_proc_limit(const std::shared_ptr<Job>& job, int64_t now_ms) {
    if (!inspector_) return;
    
    int proc_count = inspector_->count_processes_in_pgid(job->pgid);
    
    // If error (-1) or within limit, do nothing
    if (proc_count < 0 || proc_count <= policy_.max_processes_per_job) {
        return;
    }
    
    // Exceeded limit: send SIGTERM if not already sent
    if (!job->kill_signal_sent) {
        spawner_->signal_pgid(job->pgid, SIGTERM);
        job->kill_signal_sent = true;
        job->sigterm_sent_at_ms = now_ms;
    }
}

void JobRunner::escalate_kill(const std::shared_ptr<Job>& job, int64_t now_ms) {
    if (now_ms - job->sigterm_sent_at_ms >= policy_.kill_grace_ms) {
        // Grace period expired, send SIGKILL
        spawner_->signal_pgid(job->pgid, SIGKILL);
        job->status = JobStatus::PROC_LIMIT;
        job->finished_at_ms = now_ms;
        job->exit_code = -1;
    }
}

} // namespace heidi
