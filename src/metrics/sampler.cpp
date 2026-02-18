#include "heidi-kernel/metrics.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <thread>

namespace fs = std::filesystem;

namespace heidi {

MetricsHistory::MetricsHistory(const std::string& state_dir, size_t max_file_size, size_t max_files)
    : state_dir_(state_dir), max_file_size_(max_file_size), max_files_(max_files) {
  fs::create_directories(state_dir);
}

void MetricsHistory::append(const SystemMetrics& metrics) {
  std::string current_file = state_dir_ + "/metrics.log";

  // Check if rotation needed
  if (fs::exists(current_file) && fs::file_size(current_file) > max_file_size_) {
    rotate_files();
  }

  std::ofstream file(current_file, std::ios::app);
  file << metrics.timestamp << "," << metrics.cpu_usage_percent << "," << metrics.mem.total << ","
       << metrics.mem.free << "," << metrics.mem.available << "\n";
}

std::vector<SystemMetrics> MetricsHistory::tail(size_t n) {
  std::vector<SystemMetrics> result;
  std::string current_file = state_dir_ + "/metrics.log";

  if (!fs::exists(current_file))
    return result;

  std::ifstream file(current_file);
  std::string line;
  std::vector<std::string> lines;

  while (std::getline(file, line)) {
    lines.push_back(line);
  }

  size_t start = lines.size() > n ? lines.size() - n : 0;
  for (size_t i = start; i < lines.size(); ++i) {
    std::istringstream iss(lines[i]);
    SystemMetrics m;
    char comma;
    iss >> m.timestamp >> comma >> m.cpu_usage_percent >> comma >> m.mem.total >> comma >>
        m.mem.free >> comma >> m.mem.available;
    result.push_back(m);
  }

  return result;
}

void MetricsHistory::rotate_files() {
  std::string base = state_dir_ + "/metrics.log";

  // Remove oldest
  std::string oldest = state_dir_ + "/metrics.log." + std::to_string(max_files_ - 1);
  fs::remove(oldest);

  // Shift existing
  for (int i = max_files_ - 2; i >= 0; --i) {
    std::string from = state_dir_ + "/metrics.log" + (i == 0 ? "" : "." + std::to_string(i));
    std::string to = state_dir_ + "/metrics.log." + std::to_string(i + 1);
    if (fs::exists(from)) {
      fs::rename(from, to);
    }
  }
}

CpuStats MetricsSampler::read_cpu_stats() {
  CpuStats stats;
  std::ifstream file("/proc/stat");
  std::string line;
  while (std::getline(file, line)) {
    if (line.substr(0, 3) == "cpu") {
      std::istringstream iss(line);
      std::string cpu;
      iss >> cpu >> stats.user >> stats.nice >> stats.system >> stats.idle >> stats.iowait >>
          stats.irq >> stats.softirq;
      break; // Only total cpu for now
    }
  }
  return stats;
}

MemStats MetricsSampler::read_mem_stats() {
  MemStats stats;
  std::ifstream file("/proc/meminfo");
  std::string line;
  while (std::getline(file, line)) {
    std::istringstream iss(line);
    std::string key;
    uint64_t value;
    std::string unit;
    iss >> key >> value >> unit;
    if (key == "MemTotal:")
      stats.total = value;
    else if (key == "MemFree:")
      stats.free = value;
    else if (key == "MemAvailable:")
      stats.available = value;
    else if (key == "Buffers:")
      stats.buffers = value;
    else if (key == "Cached:")
      stats.cached = value;
  }
  return stats;
}

MetricsSampler::MetricsSampler() = default;

SystemMetrics MetricsSampler::sample() {
  SystemMetrics metrics;
  metrics.timestamp = std::chrono::duration_cast<std::chrono::seconds>(
                          std::chrono::system_clock::now().time_since_epoch())
                          .count();

  CpuStats current = read_cpu_stats();
  MemStats mem = read_mem_stats();

  if (!first_sample_) {
    uint64_t prev_total = prev_cpu_.user + prev_cpu_.nice + prev_cpu_.system + prev_cpu_.idle +
                          prev_cpu_.iowait + prev_cpu_.irq + prev_cpu_.softirq;
    uint64_t current_total = current.user + current.nice + current.system + current.idle +
                             current.iowait + current.irq + current.softirq;

    uint64_t delta_total = current_total - prev_total;
    uint64_t delta_idle = current.idle - prev_cpu_.idle;

    if (delta_total > 0) {
      metrics.cpu_usage_percent = 100.0 * (delta_total - delta_idle) / delta_total;
    }
  }

  prev_cpu_ = current;
  metrics.mem = mem;
  first_sample_ = false;

  return metrics;
}

} // namespace heidi