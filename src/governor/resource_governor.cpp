#include "heidi-kernel/resource_governor.h"

#include <cmath>

namespace heidi {

ResourceGovernor::ResourceGovernor(const GovernorPolicy& policy) : policy_(policy) {}

GovernorResult ResourceGovernor::decide(double cpu_pct, double mem_pct, int running_jobs,
                                        int queued_jobs) const {
  GovernorResult result;

  // Rule 1: If queue is full, reject
  if (queued_jobs >= policy_.max_queue_depth) {
    result.decision = GovernorDecision::REJECT_QUEUE_FULL;
    result.reason = BlockReason::QUEUE_FULL;
    result.retry_after_ms = policy_.cooldown_ms;
    return result;
  }

  // Rule 2: If running limit reached, hold
  if (running_jobs >= policy_.max_running_jobs) {
    result.decision = GovernorDecision::HOLD_QUEUE;
    result.reason = BlockReason::RUNNING_LIMIT;
    result.retry_after_ms = policy_.min_start_gap_ms;
    return result;
  }

  // Rule 3: If CPU high, hold
  if (cpu_pct >= policy_.cpu_high_watermark_pct) {
    result.decision = GovernorDecision::HOLD_QUEUE;
    result.reason = BlockReason::CPU_HIGH;
    result.retry_after_ms = policy_.cooldown_ms;
    return result;
  }

  // Rule 4: If MEM high, hold
  if (mem_pct >= policy_.mem_high_watermark_pct) {
    result.decision = GovernorDecision::HOLD_QUEUE;
    result.reason = BlockReason::MEM_HIGH;
    result.retry_after_ms = policy_.cooldown_ms;
    return result;
  }

  // Rule 5: Otherwise, start
  result.decision = GovernorDecision::START_NOW;
  result.reason = BlockReason::NONE;
  result.retry_after_ms = 0;
  return result;
}

void ResourceGovernor::update_policy(const GovernorPolicy& policy) {
  policy_ = policy;
}

const GovernorPolicy& ResourceGovernor::get_policy() const {
  return policy_;
}

PolicyUpdateResult ResourceGovernor::validate_and_update(const GovernorPolicy& policy) {
  PolicyUpdateResult result;
  result.success = true;
  result.effective_policy = policy_;

  // Validate max_running_jobs
  if (policy.max_running_jobs < 1 || policy.max_running_jobs > 1000) {
    result.errors.push_back({"max_running_jobs", "must be between 1 and 1000"});
    result.success = false;
  }

  // Validate max_queue_depth
  if (policy.max_queue_depth < 1 || policy.max_queue_depth > 10000) {
    result.errors.push_back({"max_queue_depth", "must be between 1 and 10000"});
    result.success = false;
  }

  // Validate cpu_high_watermark_pct
  if (std::isnan(policy.cpu_high_watermark_pct) || policy.cpu_high_watermark_pct < 0.0 ||
      policy.cpu_high_watermark_pct > 100.0) {
    result.errors.push_back({"cpu_high_watermark_pct", "must be between 0 and 100"});
    result.success = false;
  }

  // Validate mem_high_watermark_pct
  if (std::isnan(policy.mem_high_watermark_pct) || policy.mem_high_watermark_pct < 0.0 ||
      policy.mem_high_watermark_pct > 100.0) {
    result.errors.push_back({"mem_high_watermark_pct", "must be between 0 and 100"});
    result.success = false;
  }

  // Validate cooldown_ms - no validation needed for uint64_t (always >= 0)

  // Validate min_start_gap_ms - no validation needed for uint64_t (always >= 0)

  // Only update if validation passes
  if (result.success) {
    policy_ = policy;
    result.effective_policy = policy_;
  }

  return result;
}

} // namespace heidi