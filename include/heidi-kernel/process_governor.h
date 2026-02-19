#pragma once

#include "heidi-kernel/gov_rule.h"

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>

namespace heidi {
namespace gov {

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
    uint64_t last_err = 0;
    std::string last_err_detail;
    size_t rules_count = 0;
  };
  Stats get_stats() const;

private:
  void apply_loop();

  ApplyResult apply_rules(int32_t pid, const GovApplyMsg& msg);

  ApplyResult apply_affinity(int32_t pid, const std::string& affinity);
  ApplyResult apply_nice(int32_t pid, int8_t nice);
  ApplyResult apply_rlimit(int32_t pid, const RlimPolicy& rlim);
  ApplyResult apply_oom_score_adj(int32_t pid, int oom_score_adj);

  bool is_process_alive(int32_t pid);

  static constexpr size_t kQueueCapacity = 256;
  static constexpr size_t kMaxRules = 1024;

  std::queue<GovApplyMsg> ingress_queue_;
  mutable std::mutex queue_mutex_;
  std::atomic<bool> running_{false};
  std::thread apply_thread_;

  std::unordered_map<int32_t, GovApplyMsg> rules_;
  mutable std::mutex rules_mutex_;

  Stats stats_;
};

} // namespace gov
} // namespace heidi
