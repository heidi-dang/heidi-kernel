#include "heidi-kernel/group_policy_store.h"

#include <chrono>
#include <cstring>

namespace heidi {
namespace gov {

namespace {

uint64_t get_current_time_ns() {
  return std::chrono::duration_cast<std::chrono::nanoseconds>(
             std::chrono::steady_clock::now().time_since_epoch())
      .count();
}

} // namespace

uint64_t GroupPolicyStore::get_time() const {
  if (test_seq_ != 0) {
    return test_seq_;
  }
  return std::chrono::duration_cast<std::chrono::nanoseconds>(
             std::chrono::steady_clock::now().time_since_epoch())
      .count();
}

void GroupPolicyStore::tick() {
  if (test_seq_ != 0) {
    test_seq_ += kTimeIncrement;
  }
}

bool GroupPolicy::has_any_policy() const {
  return cpu_max_pct.has_value() || cpu_quota_us.has_value() || cpu_period_us.has_value() ||
         mem_max_bytes.has_value() || mem_high_bytes.has_value() || pids_max.has_value() ||
         default_action.has_value() || apply_deadline_ms.has_value();
}

bool GroupPolicyStore::upsert_group(const char* group_id, const GovApplyMsg& msg) {
  if (!msg.group) {
    return false;
  }

  const char* gid = msg.group->c_str();
  size_t gid_len = msg.group->size();
  if (gid_len > kMaxGroupIdLen) {
    return false;
  }

  for (size_t i = 0; i < kMaxGroups; ++i) {
    if (groups_[i].in_use &&
        std::strncmp(groups_[i].policy.group_id.data(), gid, kMaxGroupIdLen) == 0) {
      groups_[i].policy.last_update_ns = get_time();

      if (msg.cpu) {
        if (msg.cpu->max_pct)
          groups_[i].policy.cpu_max_pct = msg.cpu->max_pct;
      }
      if (msg.mem) {
        if (msg.mem->max_bytes)
          groups_[i].policy.mem_max_bytes = msg.mem->max_bytes;
        if (msg.mem->high_bytes)
          groups_[i].policy.mem_high_bytes = msg.mem->high_bytes;
      }
      if (msg.pids) {
        if (msg.pids->max)
          groups_[i].policy.pids_max = msg.pids->max;
      }
      if (msg.action) {
        groups_[i].policy.default_action = msg.action;
      }
      if (msg.timeouts && msg.timeouts->apply_deadline_ms) {
        groups_[i].policy.apply_deadline_ms = msg.timeouts->apply_deadline_ms;
      }
      return true;
    }
  }

  for (size_t i = 0; i < kMaxGroups; ++i) {
    if (!groups_[i].in_use) {
      groups_[i] = GroupEntry{};
      std::strncpy(groups_[i].policy.group_id.data(), gid, gid_len);
      groups_[i].policy.group_id[gid_len] = '\0';
      groups_[i].policy.last_update_ns = get_time();
      groups_[i].in_use = true;
      group_count_++;
      return true;
    }
  }

  evict_oldest_group();
  stats_.group_evictions++;

  for (size_t i = 0; i < kMaxGroups; ++i) {
    if (!groups_[i].in_use) {
      groups_[i] = GroupEntry{};
      std::strncpy(groups_[i].policy.group_id.data(), gid, gid_len);
      groups_[i].policy.group_id[gid_len] = '\0';
      groups_[i].policy.last_update_ns = get_time();
      groups_[i].in_use = true;
      group_count_++;
      return true;
    }
  }

  return false;
}

bool GroupPolicyStore::map_pid_to_group(int32_t pid, const char* group_id) {
  for (size_t i = 0; i < kMaxPidGroupMap; ++i) {
    if (pid_map_[i].in_use && static_cast<int32_t>(i) == pid) {
      std::strncpy(pid_map_[i].group_id.data(), group_id, kMaxGroupIdLen);
      pid_map_[i].group_id[kMaxGroupIdLen] = '\0';
      pid_map_[i].last_seen_ns = get_time();
      return true;
    }
  }

  for (size_t i = 0; i < kMaxPidGroupMap; ++i) {
    if (!pid_map_[i].in_use) {
      pid_map_[i] = PidEntry{};
      std::strncpy(pid_map_[i].group_id.data(), group_id, kMaxGroupIdLen);
      pid_map_[i].group_id[kMaxGroupIdLen] = '\0';
      pid_map_[i].last_seen_ns = get_time();
      pid_map_[i].in_use = true;
      pid_map_count_++;
      return true;
    }
  }

  evict_oldest_pid_entry();
  stats_.pidmap_evictions++;

  for (size_t i = 0; i < kMaxPidGroupMap; ++i) {
    if (!pid_map_[i].in_use) {
      pid_map_[i] = PidEntry{};
      std::strncpy(pid_map_[i].group_id.data(), group_id, kMaxGroupIdLen);
      pid_map_[i].group_id[kMaxGroupIdLen] = '\0';
      pid_map_[i].last_seen_ns = get_time();
      pid_map_[i].in_use = true;
      pid_map_count_++;
      return true;
    }
  }

  return false;
}

const GroupPolicy* GroupPolicyStore::get_group(const char* group_id) const {
  for (size_t i = 0; i < kMaxGroups; ++i) {
    if (groups_[i].in_use &&
        std::strncmp(groups_[i].policy.group_id.data(), group_id, kMaxGroupIdLen) == 0) {
      return &groups_[i].policy;
    }
  }
  return nullptr;
}

const char* GroupPolicyStore::get_group_for_pid(int32_t pid) const {
  for (size_t i = 0; i < kMaxPidGroupMap; ++i) {
    if (pid_map_[i].in_use && static_cast<int32_t>(i) == pid) {
      return pid_map_[i].group_id.data();
    }
  }
  return nullptr;
}

GroupPolicyStore::Stats GroupPolicyStore::get_stats() const {
  Stats s = stats_;
  s.group_count = group_count_;
  s.pid_group_map_count = pid_map_count_;
  return s;
}

void GroupPolicyStore::clear() {
  for (auto& g : groups_) {
    g = GroupEntry{};
  }
  for (auto& p : pid_map_) {
    p = PidEntry{};
  }
  group_count_ = 0;
  pid_map_count_ = 0;
  stats_ = Stats();
}

void GroupPolicyStore::evict_oldest_group() {
  uint64_t oldest = UINT64_MAX;
  size_t oldest_idx = 0;
  for (size_t i = 0; i < kMaxGroups; ++i) {
    if (groups_[i].in_use && groups_[i].policy.last_update_ns < oldest) {
      oldest = groups_[i].policy.last_update_ns;
      oldest_idx = i;
    }
  }
  groups_[oldest_idx].in_use = false;
  if (group_count_ > 0)
    group_count_--;
}

void GroupPolicyStore::evict_oldest_pid_entry() {
  uint64_t oldest = UINT64_MAX;
  size_t oldest_idx = 0;
  for (size_t i = 0; i < kMaxPidGroupMap; ++i) {
    if (pid_map_[i].in_use && pid_map_[i].last_seen_ns < oldest) {
      oldest = pid_map_[i].last_seen_ns;
      oldest_idx = i;
    }
  }
  pid_map_[oldest_idx].in_use = false;
  if (pid_map_count_ > 0)
    pid_map_count_--;
}

} // namespace gov
} // namespace heidi
