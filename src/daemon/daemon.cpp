#include "heidi-kernel/daemon.h"
#include "heidi-kernel/ipc.h"
#include "heidi-kernel/metrics.h"
#include "heidi-kernel/job.h"

#include <csignal>
#include <iostream>
#include <chrono>
#include <memory>

namespace heidi {

static volatile bool g_running = true;

void signal_handler(int sig) {
    g_running = false;
}

Daemon::Daemon(const std::string& socket_path, const std::string& state_dir)
    : socket_path_(socket_path), state_dir_(state_dir), history_(new MetricsHistory(state_dir)), job_runner_(new JobRunner()) {
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
}

Daemon::~Daemon() {
    delete history_;
    delete job_runner_;
}

void Daemon::run() {
    running_ = true;
    
    // Start sampling thread
    sampler_thread_ = std::thread(&Daemon::sampling_thread, this);
    
    // Start job runner
    job_runner_->start();
    
    UnixSocketServer server(socket_path_);
    server.set_request_handler([this](const std::string& request) -> std::string {
        if (request == "ping") {
            return "pong\n";
        } else if (request == "status") {
            auto metrics = get_latest_metrics();
            std::ostringstream oss;
            oss << "status\nversion: 0.1.0\ncpu: " << metrics.cpu_usage_percent << "%\nmem_total: " << metrics.mem.total << "\nmem_free: " << metrics.mem.free << "\n";
            return oss.str();
        } else if (request == "metrics latest") {
            auto metrics = get_latest_metrics();
            std::ostringstream oss;
            oss << "metrics latest\ncpu: " << metrics.cpu_usage_percent << "%\nmem_total: " << metrics.mem.total << "\nmem_free: " << metrics.mem.free << "\n";
            return oss.str();
        } else if (request.find("metrics tail") == 0) {
            size_t n = 5; // default
            auto tail = get_metrics_tail(n);
            std::ostringstream oss;
            oss << "metrics tail\n";
            for (const auto& m : tail) {
                oss << m.timestamp << "," << m.cpu_usage_percent << "," << m.mem.total << "," << m.mem.free << "\n";
            }
            return oss.str();
        } else if (request.find("job run ") == 0) {
            std::string command = request.substr(8); // "job run " is 8 chars
            std::string job_id = job_runner_->submit_job(command);
            return "job submitted\nid: " + job_id + "\n";
        } else if (request.find("job status ") == 0) {
            std::string job_id = request.substr(11); // "job status " is 11 chars
            auto job = job_runner_->get_job_status(job_id);
            if (!job) {
                return "error\njob not found\n";
            }
            std::ostringstream oss;
            oss << "job status\nid: " << job->id << "\nstatus: ";
            switch (job->status) {
                case JobStatus::PENDING: oss << "pending"; break;
                case JobStatus::RUNNING: oss << "running"; break;
                case JobStatus::COMPLETED: oss << "completed"; break;
                case JobStatus::FAILED: oss << "failed"; break;
                case JobStatus::CANCELLED: oss << "cancelled"; break;
            }
            oss << "\nexit_code: " << job->exit_code << "\n";
            if (!job->output.empty()) {
                oss << "output:\n" << job->output;
            }
            if (!job->error.empty()) {
                oss << "error:\n" << job->error;
            }
            return oss.str();
        } else if (request == "job status") {
            auto jobs = job_runner_->get_recent_jobs(10);
            std::ostringstream oss;
            oss << "job status\n";
            for (const auto& j : jobs) {
                oss << j->id << ",";
                switch (j->status) {
                    case JobStatus::PENDING: oss << "pending"; break;
                    case JobStatus::RUNNING: oss << "running"; break;
                    case JobStatus::COMPLETED: oss << "completed"; break;
                    case JobStatus::FAILED: oss << "failed"; break;
                    case JobStatus::CANCELLED: oss << "cancelled"; break;
                }
                oss << "," << j->exit_code << "\n";
            }
            return oss.str();
        } else if (request.find("job tail ") == 0) {
            std::string job_id = request.substr(9); // "job tail " is 9 chars
            auto job = job_runner_->get_job_status(job_id);
            if (!job) {
                return "error\njob not found\n";
            }
            std::ostringstream oss;
            oss << "job tail\nid: " << job->id << "\noutput:\n" << job->output << "\nerror:\n" << job->error << "\n";
            return oss.str();
        } else if (request.find("job cancel ") == 0) {
            std::string job_id = request.substr(12); // "job cancel " is 12 chars
            if (job_runner_->cancel_job(job_id)) {
                return "job cancelled\nid: " + job_id + "\n";
            } else {
                return "error\njob not found\n";
            }
        } else {
            return "error\n";
        }
    });
    
    std::cout << "Daemon started on " << socket_path_ << std::endl;
    
    while (running_ && g_running) {
        server.serve_forever();
    }
    
    server.stop();
    
    // Stop job runner
    job_runner_->stop();
    
    // Stop sampling
    {
        std::unique_lock<std::mutex> lock(cv_mutex_);
        running_ = false;
        cv_.notify_all();
    }
    if (sampler_thread_.joinable()) {
        sampler_thread_.join();
    }
    
    std::cout << "Daemon stopped" << std::endl;
}

void Daemon::stop() {
    running_ = false;
    cv_.notify_all();
}

SystemMetrics Daemon::get_latest_metrics() const {
    std::unique_lock<std::mutex> lock(metrics_mutex_);
    return latest_metrics_;
}

std::vector<SystemMetrics> Daemon::get_metrics_tail(size_t n) const {
    return history_->tail(n);
}

void Daemon::sampling_thread() {
    MetricsSampler sampler;
    
    while (running_) {
        // Sample
        SystemMetrics metrics = sampler.sample();
        
        // Update latest
        {
            std::unique_lock<std::mutex> lock(metrics_mutex_);
            latest_metrics_ = metrics;
        }
        
        // Write to disk
        history_->append(metrics);
        
        // Sleep for 1 second
        {
            std::unique_lock<std::mutex> lock(cv_mutex_);
            if (cv_.wait_for(lock, std::chrono::seconds(1), [this]() { return !running_; })) {
                break; // Woken by stop
            }
        }
    }
}

} // namespace heidi