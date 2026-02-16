#pragma once

#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <unordered_map>
#include <memory>
#include <atomic>

namespace heidi {

enum class JobStatus {
    PENDING,
    RUNNING,
    COMPLETED,
    FAILED,
    CANCELLED
};

struct Job {
    std::string id;
    std::string command;
    JobStatus status = JobStatus::PENDING;
    int exit_code = -1;
    std::string output;
    std::string error;
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point started_at;
    std::chrono::system_clock::time_point finished_at;
};

class JobRunner {
public:
    JobRunner(size_t max_concurrent_jobs = 10);
    ~JobRunner();

    void start();
    void stop();

    std::string submit_job(const std::string& command);
    bool cancel_job(const std::string& job_id);
    std::shared_ptr<Job> get_job_status(const std::string& job_id) const;
    std::vector<std::shared_ptr<Job>> get_recent_jobs(size_t limit = 10) const;

private:
    void worker_thread();
    void execute_job(std::shared_ptr<Job> job);

    size_t max_concurrent_;
    std::atomic<bool> running_{false};
    std::queue<std::shared_ptr<Job>> job_queue_;
    std::unordered_map<std::string, std::shared_ptr<Job>> jobs_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::vector<std::thread> worker_threads_;
};

} // namespace heidi