#include "heidi-kernel/resource_governor.h"

#include <gtest/gtest.h>

namespace heidi {

class ResourceGovernorTest : public ::testing::Test {
protected:
  ResourceGovernor governor_;
};

TEST_F(ResourceGovernorTest, DefaultPolicy) {
  const auto& policy = governor_.get_policy();
  EXPECT_EQ(policy.max_running_jobs, 10);
  EXPECT_EQ(policy.max_queue_depth, 100);
  EXPECT_DOUBLE_EQ(policy.cpu_high_watermark_pct, 85.0);
  EXPECT_DOUBLE_EQ(policy.mem_high_watermark_pct, 90.0);
  EXPECT_EQ(policy.cooldown_ms, 1000);
  EXPECT_EQ(policy.min_start_gap_ms, 100);
}

TEST_F(ResourceGovernorTest, StartNowWhenAllGood) {
  auto result = governor_.decide(50.0, 60.0, 5, 0);
  EXPECT_EQ(result.decision, GovernorDecision::START_NOW);
  EXPECT_EQ(result.reason, BlockReason::NONE);
  EXPECT_EQ(result.retry_after_ms, 0);
}

TEST_F(ResourceGovernorTest, RejectWhenQueueFull) {
  auto result = governor_.decide(50.0, 60.0, 5, 100);
  EXPECT_EQ(result.decision, GovernorDecision::REJECT_QUEUE_FULL);
  EXPECT_EQ(result.reason, BlockReason::QUEUE_FULL);
  EXPECT_EQ(result.retry_after_ms, 1000);
}

TEST_F(ResourceGovernorTest, HoldWhenRunningLimitReached) {
  auto result = governor_.decide(50.0, 60.0, 10, 5);
  EXPECT_EQ(result.decision, GovernorDecision::HOLD_QUEUE);
  EXPECT_EQ(result.reason, BlockReason::RUNNING_LIMIT);
  EXPECT_EQ(result.retry_after_ms, 100);
}

TEST_F(ResourceGovernorTest, HoldWhenCpuHigh) {
  auto result = governor_.decide(90.0, 60.0, 5, 5);
  EXPECT_EQ(result.decision, GovernorDecision::HOLD_QUEUE);
  EXPECT_EQ(result.reason, BlockReason::CPU_HIGH);
  EXPECT_EQ(result.retry_after_ms, 1000);
}

TEST_F(ResourceGovernorTest, HoldWhenMemHigh) {
  auto result = governor_.decide(50.0, 95.0, 5, 5);
  EXPECT_EQ(result.decision, GovernorDecision::HOLD_QUEUE);
  EXPECT_EQ(result.reason, BlockReason::MEM_HIGH);
  EXPECT_EQ(result.retry_after_ms, 1000);
}

TEST_F(ResourceGovernorTest, PriorityOrder_QueueFullTakesPrecedence) {
  // Even if CPU is low and running is under limit, queue full should reject
  auto result = governor_.decide(50.0, 60.0, 5, 100);
  EXPECT_EQ(result.decision, GovernorDecision::REJECT_QUEUE_FULL);
  EXPECT_EQ(result.reason, BlockReason::QUEUE_FULL);
}

TEST_F(ResourceGovernorTest, PriorityOrder_RunningLimitBeforeResourceChecks) {
  // Running limit should be checked before CPU/MEM
  auto result = governor_.decide(90.0, 95.0, 10, 5);
  EXPECT_EQ(result.decision, GovernorDecision::HOLD_QUEUE);
  EXPECT_EQ(result.reason, BlockReason::RUNNING_LIMIT);
}

TEST_F(ResourceGovernorTest, UpdatePolicy) {
  GovernorPolicy new_policy;
  new_policy.max_running_jobs = 5;
  new_policy.cpu_high_watermark_pct = 70.0;

  governor_.update_policy(new_policy);

  const auto& updated = governor_.get_policy();
  EXPECT_EQ(updated.max_running_jobs, 5);
  EXPECT_DOUBLE_EQ(updated.cpu_high_watermark_pct, 70.0);
}

TEST_F(ResourceGovernorTest, ValidateAndUpdate_ValidUpdate) {
  GovernorPolicy new_policy;
  new_policy.max_running_jobs = 20;
  new_policy.max_queue_depth = 200;
  new_policy.cpu_high_watermark_pct = 80.0;

  auto result = governor_.validate_and_update(new_policy);

  EXPECT_TRUE(result.success);
  EXPECT_EQ(result.effective_policy.max_running_jobs, 20);
  EXPECT_EQ(result.effective_policy.max_queue_depth, 200);
  EXPECT_DOUBLE_EQ(result.effective_policy.cpu_high_watermark_pct, 80.0);

  // Verify in-memory policy was updated
  const auto& current = governor_.get_policy();
  EXPECT_EQ(current.max_running_jobs, 20);
}

TEST_F(ResourceGovernorTest, ValidateAndUpdate_InvalidRange) {
  GovernorPolicy new_policy;
  new_policy.max_running_jobs = -1; // Invalid
  new_policy.max_queue_depth = 100;

  auto result = governor_.validate_and_update(new_policy);

  EXPECT_FALSE(result.success);
  EXPECT_EQ(result.errors.size(), 1);
  EXPECT_EQ(result.errors[0].field, "max_running_jobs");
  EXPECT_EQ(result.errors[0].message, "must be between 1 and 1000");

  // Verify in-memory policy was NOT updated
  const auto& current = governor_.get_policy();
  EXPECT_EQ(current.max_running_jobs, 10); // Still default
}

TEST_F(ResourceGovernorTest, ValidateAndUpdate_InvalidType) {
  GovernorPolicy new_policy;
  new_policy.max_running_jobs = 10;
  new_policy.cpu_high_watermark_pct = 999.0; // Out of range

  auto result = governor_.validate_and_update(new_policy);

  EXPECT_FALSE(result.success);
  EXPECT_EQ(result.errors.size(), 1);
  EXPECT_EQ(result.errors[0].field, "cpu_high_watermark_pct");

  // Verify in-memory policy was NOT updated
  const auto& current = governor_.get_policy();
  EXPECT_DOUBLE_EQ(current.cpu_high_watermark_pct, 85.0); // Still default
}

TEST_F(ResourceGovernorTest, ValidateAndUpdate_MultipleErrors) {
  GovernorPolicy new_policy;
  new_policy.max_running_jobs = -5;          // Invalid
  new_policy.max_queue_depth = 20000;        // Invalid (> 10000)
  new_policy.cpu_high_watermark_pct = 150.0; // Invalid (> 100)

  auto result = governor_.validate_and_update(new_policy);

  EXPECT_FALSE(result.success);
  EXPECT_EQ(result.errors.size(), 3);

  // Verify in-memory policy was NOT updated
  const auto& current = governor_.get_policy();
  EXPECT_EQ(current.max_running_jobs, 10); // Still default
  EXPECT_EQ(current.max_queue_depth, 100); // Still default
}

} // namespace heidi