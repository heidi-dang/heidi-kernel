#include "heidi-kernel/process_governor.h"

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <fcntl.h>
#include <sched.h>
#include <sstream>
#include <sys/epoll.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef __NR_pidfd_open
#include <sys/syscall.h>
#endif

namespace heidi {
namespace gov {

namespace {

constexpr uint64_t kMaxStartTimeNs = 100000000ULL;

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

    if (dash_pos != std::string_view::npos &&
        dash_pos < (comma_pos == std::string_view::npos ? part.size() : comma_pos)) {
      std::string left = part.substr(0, dash_pos);
      std::string right = part.substr(dash_pos + 1);
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

uint64_t get_current_time_ns() {
  return std::chrono::duration_cast<std::chrono::nanoseconds>(
             std::chrono::steady_clock::now().time_since_epoch())
      .count();
}

uint64_t get_proc_start_time_ns(pid_t pid) {
  char path[64];
  snprintf(path, sizeof(path), "/proc/%d/stat", pid);
  FILE* f = fopen(path, "r");
  if (!f) {
    return 0;
  }

  char buf[1024];
  if (!fgets(buf, sizeof(buf), f)) {
    fclose(f);
    return 0;
  }
  fclose(f);

  char* p = buf;
  for (int i = 0; i < 21; ++i) {
    p = strchr(p, ' ');
    if (!p) {
      return 0;
    }
    ++p;
  }

  unsigned long long start_time_ticks = 0;
  if (sscanf(p, "%llu", &start_time_ticks) != 1) {
    return 0;
  }

  return start_time_ticks * 100;
}

int pidfd_open(pid_t pid) {
#ifdef __NR_pidfd_open
  return syscall(__NR_pidfd_open, pid, 0);
#else
  (void)pid;
  return -1;
#endif
}

} // namespace

ProcessGovernor::ProcessGovernor() = default;

ProcessGovernor::~ProcessGovernor() {
  stop();
}

void ProcessGovernor::start() {
  if (running_.load()) {
    return;
  }

  epoll_fd_ = epoll_create1(EPOLL_CLOEXEC);
  if (epoll_fd_ < 0) {
    return;
  }

  running_.store(true);
  apply_thread_ = std::thread(&ProcessGovernor::apply_loop, this);
  epoll_thread_ = std::thread(&ProcessGovernor::epoll_loop, this);
}

void ProcessGovernor::stop() {
  if (!running_.load()) {
    return;
  }
  running_.store(false);

  if (epoll_fd_ >= 0) {
    close(epoll_fd_);
    epoll_fd_ = -1;
  }

  if (apply_thread_.joinable()) {
    apply_thread_.join();
  }
  if (epoll_thread_.joinable()) {
    epoll_thread_.join();
  }

  std::lock_guard<std::mutex> lock(rules_mutex_);
  for (auto& [pid, rule] : rules_) {
    if (rule.handle.pidfd >= 0) {
      close(rule.handle.pidfd);
    }
  }
  rules_.clear();
}

bool ProcessGovernor::enqueue(const GovApplyMsg& msg) {
  std::lock_guard<std::mutex> lock(queue_mutex_);
  if (ingress_queue_.size() >= kQueueCapacity) {
    return false;
  }
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
  s.tracked_pids = rules_.size();
  auto store_stats = group_store_.get_stats();
  s.group_evictions = store_stats.group_evictions;
  s.pidmap_evictions = store_stats.pidmap_evictions;
  s.cgroup_unavailable_events = store_stats.cgroup_unavailable_count;
  return s;
}

bool ProcessGovernor::track_pid(int32_t pid) {
  std::lock_guard<std::mutex> lock(rules_mutex_);

  if (rules_.size() >= kMaxTrackedPids) {
    uint64_t oldest_ns = UINT64_MAX;
    int32_t oldest_pid = -1;
    for (auto& [existing_pid, rule] : rules_) {
      if (rule.handle.last_seen_ns < oldest_ns) {
        oldest_ns = rule.handle.last_seen_ns;
        oldest_pid = existing_pid;
      }
    }
    if (oldest_pid >= 0) {
      untrack_pid(oldest_pid);
      if (event_callback_) {
        GovApplyMsg dummy_msg;
        dummy_msg.pid = oldest_pid;
        event_callback_(3, dummy_msg, 0);
      }
      stats_.evicted_events++;
    }
  }

  auto it = rules_.find(pid);
  if (it != rules_.end()) {
    it->second.handle.last_seen_ns = get_current_time_ns();
    return true;
  }

  TrackedRule rule;
  rule.handle.pid = pid;
  rule.handle.last_seen_ns = get_current_time_ns();
  rule.handle.pidfd = -1;
  rule.handle.using_pidfd = false;

  int fd = pidfd_open(pid);
  if (fd >= 0) {
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLHUP | EPOLLRDHUP;
    ev.data.u32 = static_cast<uint32_t>(pid);
    if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &ev) == 0) {
      rule.handle.pidfd = fd;
      rule.handle.using_pidfd = true;
    } else {
      close(fd);
      fd = -1;
    }
  }

  if (fd < 0) {
    rule.handle.start_time_ns = get_proc_start_time_ns(pid);
    if (rule.handle.start_time_ns == 0) {
      return false;
    }
    rule.handle.using_pidfd = false;
  }

  rules_[pid] = rule;
  return true;
}

void ProcessGovernor::untrack_pid(int32_t pid) {
  std::lock_guard<std::mutex> lock(rules_mutex_);
  auto it = rules_.find(pid);
  if (it != rules_.end()) {
    if (it->second.handle.pidfd >= 0) {
      epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, it->second.handle.pidfd, nullptr);
      close(it->second.handle.pidfd);
    }
    rules_.erase(it);
  }
}

void ProcessGovernor::cleanup_dead_pids() {
  std::lock_guard<std::mutex> lock(rules_mutex_);
  std::vector<int32_t> to_remove;

  for (const auto& [pid, rule] : rules_) {
    if (!rule.handle.using_pidfd) {
      uint64_t current_start = get_proc_start_time_ns(pid);
      if (current_start != rule.handle.start_time_ns) {
        to_remove.push_back(pid);
      }
    }
  }

  for (int32_t pid : to_remove) {
    auto it = rules_.find(pid);
    if (it != rules_.end()) {
      if (it->second.handle.pidfd >= 0) {
        epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, it->second.handle.pidfd, nullptr);
        close(it->second.handle.pidfd);
      }
      rules_.erase(it);
      stats_.pid_exit_events++;
      if (event_callback_) {
        GovApplyMsg msg;
        msg.pid = pid;
        event_callback_(2, msg, 0);
      }
    }
  }
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

    cleanup_dead_pids();

    if (!track_pid(msg.pid)) {
      std::lock_guard<std::mutex> lock(rules_mutex_);
      stats_.messages_failed++;
      stats_.last_err = ESRCH;
      stats_.last_err_detail = "failed to track pid";
      if (event_callback_) {
        event_callback_(1, msg, ESRCH);
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
      continue;
    }

    auto result = apply_rules(msg.pid, msg);

    {
      std::lock_guard<std::mutex> lock(rules_mutex_);
      if (result.success) {
        stats_.messages_processed++;
        rules_[msg.pid].msg = msg;
      } else {
        stats_.messages_failed++;
        stats_.last_err = result.err;
        stats_.last_err_detail = result.error_detail;
      }
    }

    if (event_callback_) {
      event_callback_(result.success ? 0 : 1, msg, result.err);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
}

void ProcessGovernor::epoll_loop() {
  struct epoll_event events[kEpollMaxEvents];

  while (running_.load()) {
    int n = epoll_wait(epoll_fd_, events, kEpollMaxEvents, 10);
    if (n < 0) {
      if (errno == EINTR) {
        continue;
      }
      break;
    }

    for (int i = 0; i < n; ++i) {
      uint32_t pid = events[i].data.u32;
      if (events[i].events & (EPOLLHUP | EPOLLRDHUP | EPOLLIN)) {
        untrack_pid(static_cast<int32_t>(pid));
        {
          std::lock_guard<std::mutex> lock(rules_mutex_);
          stats_.pid_exit_events++;
        }
        if (event_callback_) {
          GovApplyMsg msg;
          msg.pid = static_cast<int32_t>(pid);
          event_callback_(2, msg, 0);
        }
      }
    }
  }
}

ApplyResult ProcessGovernor::apply_group_policy(int32_t pid, const GovApplyMsg& msg) {
  ApplyResult result;

  if (msg.group) {
    bool inserted = group_store_.upsert_group(msg.group->c_str(), msg);
    if (!inserted) {
      result.err = ENOMEM;
      result.error_detail = "failed to upsert group policy";
      return result;
    }

    auto prev_stats = group_store_.get_stats();
    group_store_.map_pid_to_group(pid, msg.group->c_str());
    auto new_stats = group_store_.get_stats();

    if (new_stats.group_evictions > prev_stats.group_evictions) {
      stats_.group_evictions++;
      if (event_callback_) {
        GovApplyMsg evict_msg;
        evict_msg.pid = pid;
        event_callback_(4, evict_msg, 0);
      }
    }
    if (new_stats.pidmap_evictions > prev_stats.pidmap_evictions) {
      stats_.pidmap_evictions++;
      if (event_callback_) {
        GovApplyMsg evict_msg;
        evict_msg.pid = pid;
        event_callback_(5, evict_msg, 0);
      }
    }

    const char* group_id = group_store_.get_group_for_pid(pid);
    if (group_id) {
      const GroupPolicy* group_policy = group_store_.get_group(group_id);
      if (group_policy) {
        auto r = apply_cgroup_policy(pid, *group_policy);
        if (!r.success) {
          return r;
        }
      }
    }
  }

  result.success = true;
  return result;
}

ApplyResult ProcessGovernor::apply_cgroup_policy(int32_t pid, const GroupPolicy& group_policy) {
  ApplyResult result;

  if (!cgroup_driver_.is_available() || !cgroup_driver_.is_enabled()) {
    uint64_t now = get_current_time_ns();
    if (now - last_cgroup_unavailable_ns_ > kCgroupUnavailableRateLimitNs) {
      last_cgroup_unavailable_ns_ = now;
      stats_.cgroup_unavailable_events++;
      if (event_callback_) {
        GovApplyMsg msg;
        msg.pid = pid;
        event_callback_(6, msg, 0);
      }
    }
    result.success = true;
    return result;
  }

  CgroupDriver::ApplyResult cr;

  CpuPolicy cpu;
  if (group_policy.cpu_max_pct) {
    cpu.max_pct = group_policy.cpu_max_pct;
  }
  if (group_policy.cpu_quota_us) {
    cpu.quota_us = group_policy.cpu_quota_us;
  }
  if (group_policy.cpu_period_us) {
    cpu.period_us = group_policy.cpu_period_us;
  }

  MemPolicy mem;
  if (group_policy.mem_max_bytes) {
    mem.max_bytes = group_policy.mem_max_bytes;
  }
  if (group_policy.mem_high_bytes) {
    mem.high_bytes = group_policy.mem_high_bytes;
  }

  PidsPolicy pids;
  if (group_policy.pids_max) {
    pids.max = group_policy.pids_max;
  }

  cr = cgroup_driver_.apply(pid, cpu, mem, pids);

  if (!cr.success) {
    result.err = cr.err;
    result.error_detail = cr.error_detail;
    return result;
  }

  result.success = true;
  return result;
}

ApplyResult ProcessGovernor::apply_rules(int32_t pid, const GovApplyMsg& msg) {
  ApplyResult result;

  if (msg.group) {
    auto r = apply_group_policy(pid, msg);
    if (!r.success) {
      return r;
    }
  }

  if (msg.cpu) {
    if (msg.cpu->affinity) {
      auto r = apply_affinity(pid, *msg.cpu->affinity);
      if (!r.success) {
        return r;
      }
      result.applied_fields = result.applied_fields | ApplyField::CPU_AFFINITY;
    }
    if (msg.cpu->nice) {
      auto r = apply_nice(pid, *msg.cpu->nice);
      if (!r.success) {
        return r;
      }
      result.applied_fields = result.applied_fields | ApplyField::CPU_NICE;
    }
  }

  if (msg.rlim) {
    auto r = apply_rlimit(pid, *msg.rlim);
    if (!r.success) {
      return r;
    }
    if (msg.rlim->nofile_soft || msg.rlim->nofile_hard) {
      result.applied_fields = result.applied_fields | ApplyField::RLIM_NOFILE;
    }
    if (msg.rlim->core_soft || msg.rlim->core_hard) {
      result.applied_fields = result.applied_fields | ApplyField::RLIM_CORE;
    }
  }

  if (msg.oom_score_adj) {
    auto r = apply_oom_score_adj(pid, *msg.oom_score_adj);
    if (!r.success) {
      return r;
    }
    result.applied_fields = result.applied_fields | ApplyField::OOM_SCORE_ADJ;
  }

  result.success = true;
  return result;
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
