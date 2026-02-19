#pragma once

#include "heidi-kernel/gov_rule.h"

#include <array>
#include <cstdint>
#include <optional>
#include <string>

namespace heidi {
namespace gov {

constexpr size_t kMaxGroups = 256;
constexpr size_t kMaxPidGroupMap = 8192;

struct GroupPolicy {
  std::array<char, kMaxGroupIdLen + 1> group_id{};
  uint64_t last_update_ns = 0;

  std::optional<uint8_t> cpu_max_pct;
  std::optional<uint32_t> cpu_quota_us;
  std::optional<uint32_t> cpu_period_us;

  std::optional<uint64_t> mem_max_bytes;
  std::optional<uint64_t> mem_high_bytes;

  std::optional<uint32_t> pids_max;

  std::optional<ViolationAction> default_action;
  std::optional<uint32_t> apply_deadline_ms;

  bool has_any_policy() const;
};

class GroupPolicyStore {
public:
  enum class EvictReason : uint8_t {
    NONE = 0,
    GROUP_EVICTED = 1,
    PIDMAP_EVICTED = 2,
  };

  struct Stats {
    size_t group_count = 0;
    size_t pid_group_map_count = 0;
    uint64_t group_evictions = 0;
    uint64_t pidmap_evictions = 0;
    uint64_t attach_failures = 0;
    uint64_t cgroup_unavailable_count = 0;
    int last_err = 0;
  };

  bool upsert_group(const char* group_id, const GovApplyMsg& msg);
  bool map_pid_to_group(int32_t pid, const char* group_id);
  const GroupPolicy* get_group(const char* group_id) const;
  const char* get_group_for_pid(int32_t pid) const;
  Stats get_stats() const;
  void clear();

  void set_time_for_test(uint64_t seq) {
    test_seq_ = seq;
  }
  uint64_t get_time_for_test() const {
    return test_seq_;
  }
  void tick();

  static constexpr uint64_t kTimeIncrement = 1000000000ULL;

private:
  uint64_t get_time() const;
  void evict_oldest_group();
  void evict_oldest_pid_entry();

  struct GroupEntry {
    GroupPolicy policy;
    bool in_use = false;
  };

  struct PidEntry {
    std::array<char, kMaxGroupIdLen + 1> group_id{};
    uint64_t last_seen_ns = 0;
    bool in_use = false;
  };

  GroupEntry groups_[kMaxGroups];
  PidEntry pid_map_[kMaxPidGroupMap];
  size_t group_count_ = 0;
  size_t pid_map_count_ = 0;
  Stats stats_;
  uint64_t test_seq_ = 0;
};

} // namespace gov
} // namespace heidi
