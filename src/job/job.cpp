#include "heidi-kernel/job.h"
#include "heidi-kernel/metrics.h"
#include "heidi-kernel/resource_governor.h"

#include <algorithm>
#include <iostream>
#include <sstream>
#include <chrono>
#include <thread>
#include <memory>
#include <unordered_map>
#include <queue>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>

namespace heidi {

class RealProcessSpawner : public IProcessSpawner {

public:

    bool spawn_job(Job& job, int* stdout_fd, int* stderr_fd) override {

        int pipe_stdout[2];

        int pipe_stderr[2];

        if (pipe(pipe_stdout) == -1 || pipe(pipe_stderr) == -1) {

            job.status = JobStatus::FAILED;

            job.error = "Failed to create pipes";

            job.finished_at = std::chrono::system_clock::now();

            return false;

        }

        // Set pipes non-blocking

        fcntl(pipe_stdout[0], F_SETFL, O_NONBLOCK);

        fcntl(pipe_stderr[0], F_SETFL, O_NONBLOCK);

        pid_t pid = fork();

        if (pid == -1) {

            close(pipe_stdout[0]);

            close(pipe_stdout[1]);

            close(pipe_stderr[0]);

            close(pipe_stderr[1]);

            job.status = JobStatus::FAILED;

            job.error = "Failed to fork";

            job.finished_at = std::chrono::system_clock::now();

            return false;

        }

        if (pid == 0) { // Child

            close(pipe_stdout[0]);

            close(pipe_stderr[0]);

            setpgid(0, 0);

            

            dup2(pipe_stdout[1], STDOUT_FILENO);

            dup2(pipe_stderr[1], STDERR_FILENO);

            close(pipe_stdout[1]);

            close(pipe_stderr[1]);

            execl("/bin/sh", "sh", "-c", job.command.c_str(), nullptr);

            _exit(1);

        } else { // Parent

            close(pipe_stdout[1]);

            close(pipe_stderr[1]);

            job.process_group = pid;

            *stdout_fd = pipe_stdout[0];

            *stderr_fd = pipe_stderr[0];

            return true;

        }

    }

};

JobRunner::JobRunner(size_t max_concurrent_jobs, IProcessSpawner* spawner, IProcessInspector* inspector)

    : max_concurrent_(max_concurrent_jobs), spawner_(spawner), inspector_(inspector) {

    if (!spawner_) {

        spawner_ = new RealProcessSpawner();

    }

    if (!inspector_) {

        inspector_ = new ProcfsProcessInspector();

    }

}

JobRunner::~JobRunner() {
    stop();
}

void JobRunner::start() {
    running_ = true;
}

void JobRunner::stop() {
    running_ = false;
    cv_.notify_all();
}

std::string JobRunner::submit_job(const std::string& command, const JobLimits& limits) {
    static std::atomic<uint64_t> job_counter{0};
    auto job = std::make_shared<Job>();
    job->id = "job_" + std::to_string(++job_counter);
    job->command = command;
    job->created_at = std::chrono::system_clock::now();
    job->max_runtime_ms = limits.max_runtime_ms;
    job->max_log_bytes = limits.max_log_bytes;
    job->max_output_line_bytes = limits.max_output_line_bytes;
    job->max_child_processes = limits.max_child_processes;

    {
        std::unique_lock<std::mutex> lock(mutex_);
        jobs_[job->id] = job;
        job_queue_.push(job);
    }
    cv_.notify_one();

    return job->id;
}

bool JobRunner::cancel_job(const std::string& job_id) {
    std::unique_lock<std::mutex> lock(mutex_);
    auto it = jobs_.find(job_id);
    if (it == jobs_.end()) {
        return false;
    }
    auto job = it->second;
    if (job->status == JobStatus::RUNNING) {
        // Send SIGTERM to process group
        if (job->process_group > 0) {
            kill(-job->process_group, SIGTERM);
            
            // Wait briefly, then SIGKILL if needed
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            
            if (kill(-job->process_group, 0) == 0) {
                kill(-job->process_group, SIGKILL);
            }
        }
        
        job->status = JobStatus::CANCELLED;
        job->finished_at = std::chrono::system_clock::now();
        job->ended_at_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            job->finished_at.time_since_epoch()).count();
    } else if (job->status == JobStatus::QUEUED) {
        job->status = JobStatus::CANCELLED;
        job->finished_at = std::chrono::system_clock::now();
        job->ended_at_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            job->finished_at.time_since_epoch()).count();
    }
    return true;
}

std::shared_ptr<Job> JobRunner::get_job_status(const std::string& job_id) const {
    std::unique_lock<std::mutex> lock(mutex_);
    auto it = jobs_.find(job_id);
    if (it == jobs_.end()) {
        return nullptr;
    }
    return it->second;
}

std::vector<std::shared_ptr<Job>> JobRunner::get_recent_jobs(size_t limit) const {
    std::unique_lock<std::mutex> lock(mutex_);
    std::vector<std::shared_ptr<Job>> all_jobs;
    for (const auto& pair : jobs_) {
        all_jobs.push_back(pair.second);
    }
    // Sort by creation time, most recent first
    std::sort(all_jobs.begin(), all_jobs.end(), 
              [](const std::shared_ptr<Job>& a, const std::shared_ptr<Job>& b) {
                  return a->created_at > b->created_at;
              });
    if (all_jobs.size() > limit) {
        all_jobs.resize(limit);
    }
    return all_jobs;
}



void JobRunner::check_job_limits(uint64_t now_ms, size_t max_jobs_to_check) {
    // std::unique_lock<std::mutex> lock(mutex_);  // Already locked by caller
    size_t checked = 0;
    
    // Get list of jobs to check (RUNNING or STARTING)
    std::vector<std::shared_ptr<Job>> running_jobs;
    for (const auto& pair : jobs_) {
        if (pair.second->status == JobStatus::RUNNING || pair.second->status == JobStatus::STARTING) {
            running_jobs.push_back(pair.second);
        }
    }
    
    size_t num_running = running_jobs.size();
    if (num_running == 0) return;
    
    // Round-robin scanning starting from scan_cursor_
    for (size_t i = 0; i < num_running && checked < max_jobs_to_check; ++i) {
        size_t idx = (scan_cursor_ + i) % num_running;
        auto job = running_jobs[idx];
        checked++;
        
        // Read from stdout_fd non-blockingly
        if (job->stdout_fd != -1) {
            char buffer[4096];
            ssize_t n = read(job->stdout_fd, buffer, sizeof(buffer));
            if (n > 0) {
                job->output.append(buffer, n);
                job->bytes_written += n;
            } else if (n == 0) {
                // EOF, close fd
                close(job->stdout_fd);
                job->stdout_fd = -1;
            }
        }
        
        // Read from stderr_fd non-blockingly
        if (job->stderr_fd != -1) {
            char buffer[4096];
            ssize_t n = read(job->stderr_fd, buffer, sizeof(buffer));
            if (n > 0) {
                job->error.append(buffer, n);
                job->bytes_written += n;
            } else if (n == 0) {
                // EOF, close fd
                close(job->stderr_fd);
                job->stderr_fd = -1;
            }
        }
        
        // Check if job finished
        if (job->process_group > 0) {
            int status;
            pid_t result = waitpid(job->process_group, &status, WNOHANG);
            if (result == job->process_group) {
                // Job finished
                job->exit_code = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
                job->finished_at = std::chrono::system_clock::now();
                job->ended_at_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                    job->finished_at.time_since_epoch()).count();
                job->status = (job->exit_code == 0) ? JobStatus::COMPLETED : JobStatus::FAILED;
                // Close any remaining fds
                if (job->stdout_fd != -1) close(job->stdout_fd);
                if (job->stderr_fd != -1) close(job->stderr_fd);
                job->stdout_fd = -1;
                job->stderr_fd = -1;
                continue;
            } else if (result == -1 && errno != ECHILD) {
                // Error, but continue
            }
        }
        
        // Check timeout
        if (enforce_job_timeout(job, now_ms)) continue;
        
        // Check log cap
        if (enforce_job_log_cap(job)) continue;
        
        // Check process cap
        enforce_job_process_cap(job, now_ms);
    }
    
    scan_cursor_ = (scan_cursor_ + checked) % num_running;
    jobs_scanned_this_tick_ = checked;
}

bool JobRunner::enforce_job_timeout(std::shared_ptr<Job> job, uint64_t now_ms) {
    uint64_t runtime_ms = now_ms - job->started_at_ms;
    
    if (runtime_ms > static_cast<uint64_t>(job->max_runtime_ms)) {
        // Send SIGTERM to process group
        if (job->process_group > 0) {
            kill(-job->process_group, SIGTERM);
        }
        
        auto now_system = std::chrono::system_clock::now();
        job->status = JobStatus::TIMEOUT;
        job->finished_at = now_system;
        job->ended_at_ms = now_ms;
        return true;
    }
    return false;
}

bool JobRunner::enforce_job_log_cap(std::shared_ptr<Job> job) {
    uint64_t total_bytes = job->output.size() + job->error.size();
    if (total_bytes > job->max_log_bytes) {
        // Truncate logs and mark as truncated
        size_t keep_bytes = job->max_log_bytes / 2;
        
        if (job->output.size() > keep_bytes) {
            job->output = job->output.substr(job->output.size() - keep_bytes);
        }
        
        size_t remaining = job->max_log_bytes - job->output.size();
        if (job->error.size() > remaining) {
            job->error = job->error.substr(job->error.size() - remaining);
        }
        
        job->log_truncated = true;
        job->bytes_written = job->max_log_bytes;
        return true;
    }
    return false;
}

bool JobRunner::enforce_job_process_cap(std::shared_ptr<Job> job, uint64_t now_ms) {
    if (!inspector_) return false;
    
    int count = inspector_->count_processes_in_pgid(job->process_group);
    if (count < 0) return false; // Unknown, skip enforcement
    
    if (count > job->max_child_processes) {
        // Terminate process group
        if (job->process_group > 0) {
            kill(-job->process_group, SIGTERM);
            // Optionally, schedule SIGKILL after grace period, but for simplicity, mark immediately
            // In a real impl, might need a kill queue or delayed action
        }
        
        auto now_system = std::chrono::system_clock::now();
        job->status = JobStatus::PROC_LIMIT;
        job->finished_at = now_system;
        job->ended_at_ms = now_ms;
        return true;
    }
    return false;
}

void JobRunner::tick(uint64_t now_ms, const SystemMetrics& metrics, size_t max_starts_per_tick, size_t max_limit_scans_per_tick) {
    std::cout << "tick called" << std::endl << std::flush;
    std::unique_lock<std::mutex> lock(mutex_);
    
    jobs_started_this_tick_ = 0;
    jobs_scanned_this_tick_ = 0;
    
    // Count current running and queued jobs
    size_t running = 0, queued = 0;
    for (const auto& pair : jobs_) {
        if (pair.second->status == JobStatus::RUNNING || pair.second->status == JobStatus::STARTING) running++;
        else if (pair.second->status == JobStatus::QUEUED) queued++;
    }
    
    double mem_pct = metrics.mem.total > 0 ? (double)metrics.mem.available / metrics.mem.total * 100.0 : 0.0;
    GovernorResult result = governor_.decide(metrics.cpu_usage_percent, mem_pct, running, queued);
    
    // Record diagnostics
    last_tick_diagnostics_.last_decision = result.decision;
    last_tick_diagnostics_.last_block_reason = result.reason;
    last_tick_diagnostics_.last_retry_after_ms = result.retry_after_ms;
    last_tick_diagnostics_.last_tick_now_ms = now_ms;
    last_tick_diagnostics_.last_tick_running = running;
    last_tick_diagnostics_.last_tick_queued = queued;
    
    size_t started = 0;
    
    // Start jobs if governor allows
    if (result.decision == GovernorDecision::START_NOW) {
        while (!job_queue_.empty() && started < max_starts_per_tick && running + started < max_concurrent_) {
            auto job = job_queue_.front();
            job_queue_.pop();
            job->status = JobStatus::STARTING;
            bool success = spawner_->spawn_job(*job, &job->stdout_fd, &job->stderr_fd);
            if (success) {
                job->status = JobStatus::RUNNING;
                job->started_at_ms = now_ms;
                started++;
            } else {
                job->status = JobStatus::FAILED;
            }
        }
    }
    
    jobs_started_this_tick_ = started;
    
    // Check limits on running jobs
    check_job_limits(now_ms, max_limit_scans_per_tick);
}

} // namespace heidi