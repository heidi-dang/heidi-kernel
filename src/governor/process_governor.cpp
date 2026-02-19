#include "heidi-kernel/process_governor.h"

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <fcntl.h>
#include <sched.h>
#include <sstream>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

namespace heidi {
namespace gov {

namespace {

bool parse_cpu_list(const std::string& affinity, std::vector<cpu_set_t>& sets) {
  sets.clear();

  std::string_view sv(affinity);
  std::vector<std::pair<int, int>> ranges;

  while (!sv.empty()) {
    size_t dash_pos = sv.find('-');
    size_t comma_pos = sv.find(',');

    std::string part;
    if (comma_pos != std::string_view::npos) {
      part = std::string(sv.substr(0, comma_pos));
      sv.remove_prefix(comma_pos + 1);
    } else {
      part = std::string(sv);
      sv = {};
    }

    part.erase(
        std::remove_if(part.begin(), part.end(), [](char c) { return c == ' ' || c == '\t'; }),
        part.end());

    if (part.empty()) {
      continue;
    }

    // Recompute dash_pos relative to part
    size_t local_dash = part.find('-');
    if (local_dash != std::string::npos) {
      std::string left = part.substr(0, local_dash);
      std::string right = part.substr(local_dash + 1);
      int start = std::stoi(left);
      int end = std::stoi(right);
      ranges.push_back({start, end});
    } else {
      int cpu = std::stoi(part);
      ranges.push_back({cpu, cpu});
    }
  }

  if (ranges.empty()) {
    return false;
  }

  for (const auto& r : ranges) {
    for (int i = r.first; i <= r.second; ++i) {
      if (i < 0 || i >= CPU_SETSIZE) {
        return false;
      }
    }
  }

  cpu_set_t mask;
  CPU_ZERO(&mask);
  for (const auto& r : ranges) {
    for (int i = r.first; i <= r.second; ++i) {
      CPU_SET(i, &mask);
    }
  }
  sets.push_back(mask);

  return true;
}

bool parse_uint8_value_simple(const std::string_view& s_in, uint8_t& out) {
  std::string s(s_in);
  try {
    unsigned long v = std::stoul(s);
    if (v > 255)
      return false;
    out = static_cast<uint8_t>(v);
    return true;
  } catch (...) {
    return false;
  }
}

bool parse_int8_value_simple(const std::string_view& s_in, int8_t& out) {
  std::string s(s_in);
  try {
    long v = std::stol(s);
    if (v < -128 || v > 127)
      return false;
    out = static_cast<int8_t>(v);
    return true;
  } catch (...) {
    return false;
  }
}

} // namespace

ProcessGovernor::ProcessGovernor() = default;

ProcessGovernor::~ProcessGovernor() {
  stop();
}

void ProcessGovernor::start() {
  if (running_.load())
    return;
  running_.store(true);
  apply_thread_ = std::thread(&ProcessGovernor::apply_loop, this);
}

void ProcessGovernor::stop() {
  if (!running_.load())
    return;
  running_.store(false);
  if (apply_thread_.joinable())
    apply_thread_.join();
}

bool ProcessGovernor::enqueue(const GovApplyMsg& msg) {
  std::lock_guard<std::mutex> lock(queue_mutex_);
  if (ingress_queue_.size() >= kQueueCapacity)
    return false;
  ingress_queue_.push(msg);
  return true;
}

size_t ProcessGovernor::queue_size() const {
  std::lock_guard<std::mutex> lock(queue_mutex_);
  return ingress_queue_.size();
}

ProcessGovernor::Stats ProcessGovernor::get_stats() const {
  std::lock_guard<std::mutex> lock(rules_mutex_);
  Stats s = stats_;
  s.rules_count = rules_.size();
  return s;
}

void ProcessGovernor::apply_loop() {
  while (running_.load()) {
    GovApplyMsg msg;
    {
      std::lock_guard<std::mutex> lock(queue_mutex_);
      if (!ingress_queue_.empty()) {
        msg = ingress_queue_.front();
        ingress_queue_.pop();
      }
    }

    if (msg.pid == 0) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
      continue;
    }

    auto result = apply_rules(msg.pid, msg);

    {
      std::lock_guard<std::mutex> lock(rules_mutex_);
      if (result.success) {
        stats_.messages_processed++;
        rules_[msg.pid] = msg;
      } else {
        stats_.messages_failed++;
        stats_.last_err = result.err;
        stats_.last_err_detail = result.error_detail;
      }
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
}

ApplyResult ProcessGovernor::apply_rules(int32_t pid, const GovApplyMsg& msg) {
  ApplyResult result;

  if (!is_process_alive(pid)) {
    result.err = ESRCH;
    result.error_detail = "process not found";
    return result;
  }

  if (msg.cpu) {
    if (msg.cpu->affinity) {
      auto r = apply_affinity(pid, *msg.cpu->affinity);
      if (!r.success)
        return r;
      result.applied_fields = result.applied_fields | ApplyField::CPU_AFFINITY;
    }
    if (msg.cpu->nice) {
      auto r = apply_nice(pid, *msg.cpu->nice);
      if (!r.success)
        return r;
      result.applied_fields = result.applied_fields | ApplyField::CPU_NICE;
    }
  }

  if (msg.rlim) {
    auto r = apply_rlimit(pid, *msg.rlim);
    if (!r.success)
      return r;
    if (msg.rlim->nofile_soft || msg.rlim->nofile_hard) {
      result.applied_fields = result.applied_fields | ApplyField::RLIM_NOFILE;
    }
    if (msg.rlim->core_soft || msg.rlim->core_hard) {
      result.applied_fields = result.applied_fields | ApplyField::RLIM_CORE;
    }
  }

  if (msg.oom_score_adj) {
    auto r = apply_oom_score_adj(pid, *msg.oom_score_adj);
    if (!r.success)
      return r;
    result.applied_fields = result.applied_fields | ApplyField::OOM_SCORE_ADJ;
  }

  result.success = true;
  return result;
}

bool ProcessGovernor::is_process_alive(int32_t pid) {
  char path[64];
  snprintf(path, sizeof(path), "/proc/%d/stat", pid);
  return access(path, F_OK) == 0;
}

ApplyResult ProcessGovernor::apply_affinity(int32_t pid, const std::string& affinity) {
  ApplyResult result;

  std::vector<cpu_set_t> sets;
  if (!parse_cpu_list(affinity, sets)) {
    result.err = EINVAL;
    result.error_detail = "invalid CPU list format";
    return result;
  }

  cpu_set_t mask = sets[0];
  int ret = sched_setaffinity(pid, sizeof(mask), &mask);
  if (ret != 0) {
    result.err = errno;
    result.error_detail = std::strerror(errno);
    return result;
  }

  result.success = true;
  return result;
}

ApplyResult ProcessGovernor::apply_nice(int32_t pid, int8_t nice) {
  ApplyResult result;

  int ret = setpriority(PRIO_PROCESS, pid, nice);
  if (ret != 0) {
    result.err = errno;
    result.error_detail = std::strerror(errno);
    return result;
  }

  result.success = true;
  return result;
}

ApplyResult ProcessGovernor::apply_rlimit(int32_t pid, const RlimPolicy& rlim) {
  ApplyResult result;

  struct rlimit rl;

  if (rlim.nofile_soft || rlim.nofile_hard) {
    if (rlim.nofile_soft) {
      rl.rlim_cur = *rlim.nofile_soft;
    } else {
      if (getrlimit(RLIMIT_NOFILE, &rl) != 0) {
        result.err = errno;
        result.error_detail = std::strerror(errno);
        return result;
      }
    }
    if (rlim.nofile_hard) {
      rl.rlim_max = *rlim.nofile_hard;
    } else {
      if (getrlimit(RLIMIT_NOFILE, &rl) != 0) {
        result.err = errno;
        result.error_detail = std::strerror(errno);
        return result;
      }
    }
    if (prlimit(pid, RLIMIT_NOFILE, &rl, nullptr) != 0) {
      result.err = errno;
      result.error_detail = "prlimit RLIMIT_NOFILE: " + std::string(std::strerror(errno));
      return result;
    }
  }

  if (rlim.core_soft || rlim.core_hard) {
    if (rlim.core_soft) {
      rl.rlim_cur = *rlim.core_soft;
    } else {
      if (getrlimit(RLIMIT_CORE, &rl) != 0) {
        result.err = errno;
        result.error_detail = std::strerror(errno);
        return result;
      }
    }
    if (rlim.core_hard) {
      rl.rlim_max = *rlim.core_hard;
    } else {
      if (getrlimit(RLIMIT_CORE, &rl) != 0) {
        result.err = errno;
        result.error_detail = std::strerror(errno);
        return result;
      }
    }
    if (prlimit(pid, RLIMIT_CORE, &rl, nullptr) != 0) {
      result.err = errno;
      result.error_detail = "prlimit RLIMIT_CORE: " + std::string(std::strerror(errno));
      return result;
    }
  }

  result.success = true;
  return result;
}

ApplyResult ProcessGovernor::apply_oom_score_adj(int32_t pid, int oom_score_adj) {
  ApplyResult result;

  char path[64];
  snprintf(path, sizeof(path), "/proc/%d/oom_score_adj", pid);

  int fd = open(path, O_WRONLY);
  if (fd < 0) {
    result.err = errno;
    result.error_detail = std::strerror(errno);
    return result;
  }

  char buf[32];
  int len = snprintf(buf, sizeof(buf), "%d", oom_score_adj);
  ssize_t written = write(fd, buf, len);
  close(fd);

  if (written != len) {
    result.err = errno;
    result.error_detail = std::strerror(errno);
    return result;
  }

  result.success = true;
  return result;
}

} // namespace gov
} // namespace heidi
