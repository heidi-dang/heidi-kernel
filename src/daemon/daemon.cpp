#include "heidi-kernel/daemon.h"
#include "heidi-kernel/ipc.h"
#include "heidi-kernel/metrics.h"
#include "heidi-kernel/job.h"
#include "heidi-kernel/resource_governor.h"

#include <csignal>
#include <iostream>
#include <chrono>
#include <memory>
#include <sys/timerfd.h>
#include <unistd.h>
#include <poll.h>
#include <sstream>
#include <algorithm>
#include <cstring>

namespace heidi {

static volatile bool g_running = true;

void signal_handler(int sig) {
    (void)sig;
    g_running = false;
}

Daemon::Daemon(const std::string& socket_path, const std::string& state_dir)
    : socket_path_(socket_path), state_dir_(state_dir), history_(new MetricsHistory(state_dir)), job_runner_(new JobRunner()), governor_(new ResourceGovernor()) {
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
}

Daemon::~Daemon() {
    delete history_;
    delete job_runner_;
    delete governor_;
}

void Daemon::run() {
    running_ = true;

    // Start sampling thread
    sampler_thread_ = std::thread(&Daemon::sampling_thread, this);

    // Start job runner
    job_runner_->start();

    // Setup timerfd for monitoring (2Hz = 500ms intervals)
    timer_fd_ = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
    if (timer_fd_ == -1) {
        throw std::runtime_error("Failed to create timerfd");
    }

    struct itimerspec timer_spec;
    timer_spec.it_interval.tv_sec = 0;
    timer_spec.it_interval.tv_nsec = 500000000; // 500ms
    timer_spec.it_value.tv_sec = 0;
    timer_spec.it_value.tv_nsec = 500000000; // 500ms initial

    if (timerfd_settime(timer_fd_, 0, &timer_spec, nullptr) == -1) {
        close(timer_fd_);
        throw std::runtime_error("Failed to set timerfd");
    }

    // Start monitor thread
    monitor_thread_ = std::thread(&Daemon::monitor_loop, this);

    UnixSocketServer server(socket_path_);
    server.set_request_handler([this](const std::string& request) -> std::string {
        if (request == "ping") {
            return "pong\n";
        } else if (request == "status") {
            auto metrics = get_latest_metrics();
            std::unique_lock<std::mutex> gov_lock(governor_mutex_);
            std::ostringstream oss;
            oss << "status\nversion: 0.1.0\ncpu: " << metrics.cpu_usage_percent << "%\nmem_total: " << metrics.mem.total << "\nmem_free: " << metrics.mem.free << "\n";
            oss << "running_jobs: " << running_jobs_ << "\n";
            oss << "queued_jobs: " << queued_jobs_ << "\n";
            oss << "rejected_jobs: " << rejected_jobs_ << "\n";
            oss << "blocked_reason: ";
            switch (blocked_reason_) {
                case BlockReason::NONE: oss << "none"; break;
                case BlockReason::CPU_HIGH: oss << "cpu_high"; break;
                case BlockReason::MEM_HIGH: oss << "mem_high"; break;
                case BlockReason::QUEUE_FULL: oss << "queue_full"; break;
                case BlockReason::RUNNING_LIMIT: oss << "running_limit"; break;
            }
            oss << "\nretry_after_ms: " << retry_after_ms_;
            oss << "\ncpu_pct: " << metrics.cpu_usage_percent;
            oss << "\nmem_pct: " << (metrics.mem.total - metrics.mem.free) * 100.0 / metrics.mem.total;
            oss << "\n";
            return oss.str();
        } else if (request.rfind("governor/policy_update ", 0) == 0) {
            // PUT policy - body follows command
            std::string json_body = request.substr(strlen("governor/policy_update "));
            return update_policy(json_body);
        } else if (request == "governor/policy") {
            std::unique_lock<std::mutex> gov_lock(governor_mutex_);
            const auto& policy = governor_->get_policy();
            std::ostringstream oss;
            oss << "governor/policy\nmax_running_jobs: " << policy.max_running_jobs << "\nmax_queue_depth: " << policy.max_queue_depth << "\n";
            oss << "cpu_high_watermark_pct: " << policy.cpu_high_watermark_pct << "\nmem_high_watermark_pct: " << policy.mem_high_watermark_pct << "\n";
            oss << "cooldown_ms: " << policy.cooldown_ms << "\nmin_start_gap_ms: " << policy.min_start_gap_ms << "\n";
            return oss.str();
        } else if (request == "governor/diagnostics") {
            std::unique_lock<std::mutex> gov_lock(governor_mutex_);
            auto diag = last_tick_diagnostics_;
            std::ostringstream oss;
            oss << "governor/diagnostics\nlast_decision: ";
            switch (diag.last_decision) {
                case GovernorDecision::START_NOW: oss << "START_NOW"; break;
                case GovernorDecision::HOLD_QUEUE: oss << "HOLD_QUEUE"; break;
                case GovernorDecision::REJECT_QUEUE_FULL: oss << "REJECT_QUEUE_FULL"; break;
            }
            oss << "\nlast_block_reason: ";
            switch (diag.last_block_reason) {
                case BlockReason::NONE: oss << "NONE"; break;
                case BlockReason::CPU_HIGH: oss << "CPU_HIGH"; break;
                case BlockReason::MEM_HIGH: oss << "MEM_HIGH"; break;
                case BlockReason::QUEUE_FULL: oss << "QUEUE_FULL"; break;
                case BlockReason::RUNNING_LIMIT: oss << "RUNNING_LIMIT"; break;
            }
            oss << "\nlast_retry_after_ms: " << diag.last_retry_after_ms << "\n";
            oss << "last_tick_now_ms: " << diag.last_tick_now_ms << "\n";
            oss << "last_tick_running: " << diag.last_tick_running << "\n";
            oss << "last_tick_queued: " << diag.last_tick_queued << "\n";
            oss << "jobs_started_this_tick: " << jobs_started_this_tick_ << "\n";
            oss << "jobs_scanned_this_tick: " << jobs_scanned_this_tick_ << "\n";
            return oss.str();
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

    // Stop monitor thread
    if (monitor_thread_.joinable()) {
        monitor_thread_.join();
    }

    // Close timer
    if (timer_fd_ >= 0) {
        close(timer_fd_);
        timer_fd_ = -1;
    }

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

std::string Daemon::update_policy(const std::string& json_body) {
    GovernorPolicy new_policy = governor_->get_policy();

    // Simple JSON parsing - only update fields that are present
    std::istringstream iss(json_body);
    std::string token;
    bool has_unknown_fields = false;

    while (std::getline(iss, token, ',')) {
        // Trim
        token.erase(token.begin(), std::find_if(token.begin(), token.end(), [](unsigned char ch) { return !std::isspace(ch) && ch != '{' && ch != '}'; }));
        token.erase(std::find_if(token.rbegin(), token.rend(), [](unsigned char ch) { return !std::isspace(ch) && ch != '{' && ch != '}'; }).base(), token.end());

        size_t colon = token.find(':');
        if (colon != std::string::npos) {
            std::string key = token.substr(0, colon);
            std::string value = token.substr(colon + 1);

            // Trim quotes from key
            if (key.front() == '"') key = key.substr(1);
            if (key.back() == '"') key = key.substr(0, key.size() - 1);

            // Trim value
            value.erase(value.begin(), std::find_if(value.begin(), value.end(), [](unsigned char ch) { return !std::isspace(ch); }));
            value.erase(std::find_if(value.rbegin(), value.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), value.end());

            // Parse known fields
            if (key == "max_running_jobs") {
                new_policy.max_running_jobs = std::stoi(value);
            } else if (key == "max_queue_depth") {
                new_policy.max_queue_depth = std::stoi(value);
            } else if (key == "cpu_high_watermark_pct") {
                new_policy.cpu_high_watermark_pct = std::stod(value);
            } else if (key == "mem_high_watermark_pct") {
                new_policy.mem_high_watermark_pct = std::stod(value);
            } else if (key == "cooldown_ms") {
                new_policy.cooldown_ms = std::stoull(value);
            } else if (key == "min_start_gap_ms") {
                new_policy.min_start_gap_ms = std::stoull(value);
            } else {
                has_unknown_fields = true;
            }
        }
    }

    std::unique_lock<std::mutex> gov_lock(governor_mutex_);
    auto result = governor_->validate_and_update(new_policy);

    if (!result.success) {
        std::ostringstream oss;
        oss << "error\nvalidation_failed\n";
        for (const auto& err : result.errors) {
            oss << err.field << ": " << err.message << "\n";
        }
        return oss.str();
    }

    if (has_unknown_fields) {
        // We ignore unknown fields, so just return success
    }

    // Return the effective policy
    const auto& policy = result.effective_policy;
    std::ostringstream oss;
    oss << "policy_updated\nmax_running_jobs: " << policy.max_running_jobs << "\nmax_queue_depth: " << policy.max_queue_depth << "\n";
    oss << "cpu_high_watermark_pct: " << policy.cpu_high_watermark_pct << "\nmem_high_watermark_pct: " << policy.mem_high_watermark_pct << "\n";
    oss << "cooldown_ms: " << policy.cooldown_ms << "\nmin_start_gap_ms: " << policy.min_start_gap_ms << "\n";
    return oss.str();
}

void Daemon::monitor_loop() {
    struct pollfd pfd;
    pfd.fd = timer_fd_;
    pfd.events = POLLIN;

    while (running_) {
        int ret = poll(&pfd, 1, 1000); // 1 second timeout

        if (ret > 0 && (pfd.revents & POLLIN)) {
            // Timer fired, consume the event
            uint64_t expirations;
            read(timer_fd_, &expirations, sizeof(expirations));

            handle_monitor_tick();
        }
    }
}

void Daemon::handle_monitor_tick() {
    uint64_t now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();

    // Get current metrics
    auto metrics = get_latest_metrics();
    double cpu_pct = metrics.cpu_usage_percent;
    double mem_pct = (metrics.mem.total - metrics.mem.free) * 100.0 / metrics.mem.total;

    std::unique_lock<std::mutex> gov_lock(governor_mutex_);

    // Update job counts (simplified - in real impl would track from job runner)
    // For now, just demonstrate the governor logic
    auto decision = governor_->decide(cpu_pct, mem_pct, running_jobs_, queued_jobs_);

    blocked_reason_ = decision.reason;
    retry_after_ms_ = decision.retry_after_ms;

    size_t started = 0;

    // If we can start jobs, try to start up to 5 per tick
    if (decision.decision == GovernorDecision::START_NOW && queued_jobs_ > 0) {
        int to_start = std::min(5, std::min(queued_jobs_, governor_->get_policy().max_running_jobs - running_jobs_));
        for (int i = 0; i < to_start; ++i) {
            // In real implementation, dequeue from job runner queue
            queued_jobs_--;
            running_jobs_++;
        }
        started = to_start;
    }

    // Check job limits with bounded work (max 10 jobs per tick)
    job_runner_->check_job_limits(10);

    last_tick_diagnostics_.last_decision = decision.decision;
    last_tick_diagnostics_.last_block_reason = decision.reason;
    last_tick_diagnostics_.last_retry_after_ms = decision.retry_after_ms;
    last_tick_diagnostics_.last_tick_now_ms = now_ms;
    last_tick_diagnostics_.last_tick_running = running_jobs_;
    last_tick_diagnostics_.last_tick_queued = queued_jobs_;
    jobs_started_this_tick_ = started;
    jobs_scanned_this_tick_ = job_runner_->get_jobs_scanned_this_tick();
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