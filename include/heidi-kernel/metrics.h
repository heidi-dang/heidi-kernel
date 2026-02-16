#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace heidi {

struct CpuStats {
  uint64_t user = 0;
  uint64_t nice = 0;
  uint64_t system = 0;
  uint64_t idle = 0;
  uint64_t iowait = 0;
  uint64_t irq = 0;
  uint64_t softirq = 0;
};

struct MemStats {
  uint64_t total = 0;
  uint64_t free = 0;
  uint64_t available = 0;
  uint64_t buffers = 0;
  uint64_t cached = 0;
};

struct SystemMetrics {
  double cpu_usage_percent = 0.0;
  MemStats mem;
  uint64_t timestamp = 0;
};

class MetricsSampler {
public:
  MetricsSampler();
  SystemMetrics sample();

private:
  CpuStats prev_cpu_;
  bool first_sample_ = true;

  CpuStats read_cpu_stats();
  MemStats read_mem_stats();
};

class MetricsHistory {
public:
  MetricsHistory(const std::string& state_dir, size_t max_file_size = 1024 * 1024,
                 size_t max_files = 5);

  void append(const SystemMetrics& metrics);
  std::vector<SystemMetrics> tail(size_t n);

private:
  void rotate_files();

  std::string state_dir_;
  size_t max_file_size_;
  size_t max_files_;
};

} // namespace heidi