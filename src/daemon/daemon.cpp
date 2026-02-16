#include "heidi-kernel/daemon.h"
#include "heidi-kernel/ipc.h"
#include "heidi-kernel/metrics.h"

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
    : socket_path_(socket_path), state_dir_(state_dir), history_(new MetricsHistory(state_dir)) {
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
}

Daemon::~Daemon() {
    delete history_;
}

void Daemon::run() {
    running_ = true;
    
    // Start sampling thread
    sampler_thread_ = std::thread(&Daemon::sampling_thread, this);
    
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
        } else {
            return "error\n";
        }
    });
    
    std::cout << "Daemon started on " << socket_path_ << std::endl;
    
    while (running_ && g_running) {
        server.serve_forever();
    }
    
    server.stop();
    
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