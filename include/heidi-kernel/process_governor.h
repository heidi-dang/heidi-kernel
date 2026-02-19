#pragma once

#include "heidi-kernel/cgroup_driver.h"
#include "heidi-kernel/gov_rule.h"
#include "heidi-kernel/group_policy_store.h"

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <thread>
#include <unordered_map>
#include <vector>

namespace heidi {
namespace gov {

struct ApplyResult {
  bool success = false;
  int err = 0;
  std::string error_detail;
  ApplyField applied_fields = ApplyField::NONE;
};

struct PidHandle {
  int pidfd = -1;
  pid_t pid = 0;
  uint64_t start_time_ns = 0;
  uint64_t last_seen_ns = 0;
  bool using_pidfd = false;
};

class ProcessGovernor {
public:
  ProcessGovernor();
  ~ProcessGovernor();

  void start();
  void stop();

  bool enqueue(const GovApplyMsg& msg);
  size_t queue_size() const;
  size_t queue_capacity() const {
    return kQueueCapacity;
  }

  struct Stats {
    uint64_t messages_processed = 0;
    uint64_t messages_failed = 0;
    uint64_t messages_dropped = 0;
    uint64_t last_err = 0;
    std::string last_err_detail;
    size_t rules_count = 0;
    size_t tracked_pids = 0;
    uint64_t pid_exit_events = 0;
    uint64_t evicted_events = 0;
    uint64_t group_evictions = 0;
    uint64_t pidmap_evictions = 0;
    uint64_t cgroup_unavailable_events = 0;
  };
  Stats get_stats() const;

  void
  set_event_callback(std::function<void(uint8_t event_type, const GovApplyMsg& msg, int err)> cb) {
    event_callback_ = std::move(cb);
  }

private:
  void apply_loop();
  void epoll_loop();

  ApplyResult apply_rules(int32_t pid, const GovApplyMsg& msg);
  ApplyResult apply_group_policy(int32_t pid, const GovApplyMsg& msg);
  ApplyResult apply_cgroup_policy(int32_t pid, const GroupPolicy& group_policy);

  ApplyResult apply_affinity(int32_t pid, const std::string& affinity);
  ApplyResult apply_nice(int32_t pid, int8_t nice);
  ApplyResult apply_rlimit(int32_t pid, const RlimPolicy& rlim);
  ApplyResult apply_oom_score_adj(int32_t pid, int oom_score_adj);

  bool track_pid(int32_t pid);
  void untrack_pid(int32_t pid);
  void cleanup_dead_pids();

  static constexpr size_t kQueueCapacity = 256;
  static constexpr size_t kMaxTrackedPids = 4096;
  static constexpr int kEpollMaxEvents = 64;

  std::queue<GovApplyMsg> ingress_queue_;
  mutable std::mutex queue_mutex_;
  std::atomic<bool> running_{false};
  std::thread apply_thread_;
  std::thread epoll_thread_;
  int epoll_fd_ = -1;

  struct TrackedRule {
    GovApplyMsg msg;
    PidHandle handle;
  };
  std::unordered_map<int32_t, TrackedRule> rules_;
  mutable std::mutex rules_mutex_;

  std::function<void(uint8_t event_type, const GovApplyMsg& msg, int err)> event_callback_;

  GroupPolicyStore group_store_;
  CgroupDriver cgroup_driver_;
  uint64_t last_cgroup_unavailable_ns_ = 0;
  static constexpr uint64_t kCgroupUnavailableRateLimitNs = 1000000000ULL;

  Stats stats_;
};

} // namespace gov
} // namespace heidi
