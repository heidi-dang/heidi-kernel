#include "heidi-kernel/job.h"

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

namespace heidi {

static std::string generate_job_id() {
    static std::atomic<uint64_t> counter{0};
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    return "job_" + std::to_string(timestamp) + "_" + std::to_string(counter++);
}

JobRunner::JobRunner(size_t max_concurrent_jobs)
    : max_concurrent_(max_concurrent_jobs) {
}

JobRunner::~JobRunner() {
    stop();
}

void JobRunner::start() {
    running_ = true;
    for (size_t i = 0; i < max_concurrent_; ++i) {
        worker_threads_.emplace_back(&JobRunner::worker_thread, this);
    }
}

void JobRunner::stop() {
    running_ = false;
    cv_.notify_all();
    for (auto& t : worker_threads_) {
        if (t.joinable()) {
            t.join();
        }
    }
    worker_threads_.clear();
}

std::string JobRunner::submit_job(const std::string& command) {
    auto job = std::make_shared<Job>();
    job->id = generate_job_id();
    job->command = command;
    job->created_at = std::chrono::system_clock::now();

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
        // Note: In a real implementation, we'd need to track PIDs and send SIGTERM
        // For now, just mark as cancelled
        job->status = JobStatus::CANCELLED;
        job->finished_at = std::chrono::system_clock::now();
    } else if (job->status == JobStatus::PENDING) {
        job->status = JobStatus::CANCELLED;
        job->finished_at = std::chrono::system_clock::now();
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

void JobRunner::worker_thread() {
    while (running_) {
        std::shared_ptr<Job> job;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait(lock, [this]() { return !running_ || !job_queue_.empty(); });
            if (!running_) break;
            if (job_queue_.empty()) continue;

            job = job_queue_.front();
            job_queue_.pop();
            job->status = JobStatus::RUNNING;
            job->started_at = std::chrono::system_clock::now();
        }

        execute_job(job);
    }
}

void JobRunner::execute_job(std::shared_ptr<Job> job) {
    // Simple fork/exec implementation
    int pipe_stdout[2];
    int pipe_stderr[2];

    if (pipe(pipe_stdout) == -1 || pipe(pipe_stderr) == -1) {
        job->status = JobStatus::FAILED;
        job->error = "Failed to create pipes";
        job->finished_at = std::chrono::system_clock::now();
        return;
    }

    pid_t pid = fork();
    if (pid == -1) {
        close(pipe_stdout[0]);
        close(pipe_stdout[1]);
        close(pipe_stderr[0]);
        close(pipe_stderr[1]);
        job->status = JobStatus::FAILED;
        job->error = "Failed to fork";
        job->finished_at = std::chrono::system_clock::now();
        return;
    }

    if (pid == 0) { // Child
        close(pipe_stdout[0]);
        close(pipe_stderr[0]);

        dup2(pipe_stdout[1], STDOUT_FILENO);
        dup2(pipe_stderr[1], STDERR_FILENO);
        close(pipe_stdout[1]);
        close(pipe_stderr[1]);

        execl("/bin/sh", "sh", "-c", job->command.c_str(), nullptr);
        _exit(1); // execl failed
    } else { // Parent
        close(pipe_stdout[1]);
        close(pipe_stderr[1]);

        // Read output
        char buffer[1024];
        ssize_t n;
        while ((n = read(pipe_stdout[0], buffer, sizeof(buffer))) > 0) {
            job->output.append(buffer, n);
        }
        while ((n = read(pipe_stderr[0], buffer, sizeof(buffer))) > 0) {
            job->error.append(buffer, n);
        }

        close(pipe_stdout[0]);
        close(pipe_stderr[0]);

        int status;
        waitpid(pid, &status, 0);

        job->exit_code = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
        job->status = (job->exit_code == 0) ? JobStatus::COMPLETED : JobStatus::FAILED;
        job->finished_at = std::chrono::system_clock::now();
    }
}

} // namespace heidi