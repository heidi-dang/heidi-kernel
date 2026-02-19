#include "heidi-kernel/group_policy_store.h"

#include <gtest/gtest.h>
#include <string>

namespace heidi {
namespace gov {

class GroupPolicyStoreEvictionTest : public ::testing::Test {
protected:
  GroupPolicyStore store_;
};

TEST_F(GroupPolicyStoreEvictionTest, Insert256GroupsEvictsOldest) {
  store_.set_time_for_test(1);

  for (int i = 0; i < 256; ++i) {
    GovApplyMsg msg;
    msg.group = "group_" + std::to_string(i);
    CpuPolicy cpu;
    cpu.max_pct = static_cast<uint8_t>(i % 100);
    msg.cpu = cpu;
    bool ok = store_.upsert_group(msg.group->c_str(), msg);
    EXPECT_TRUE(ok);
    store_.tick();
  }

  auto stats = store_.get_stats();
  EXPECT_EQ(stats.group_count, 256);

  GovApplyMsg msg257;
  msg257.group = "group_257";
  CpuPolicy cpu257;
  cpu257.max_pct = 50;
  msg257.cpu = cpu257;
  bool ok = store_.upsert_group(msg257.group->c_str(), msg257);
  EXPECT_TRUE(ok);

  stats = store_.get_stats();
  EXPECT_EQ(stats.group_count, 256);
  EXPECT_EQ(stats.group_evictions, 1);

  auto* g0 = store_.get_group("group_0");
  EXPECT_EQ(g0, nullptr);

  auto* g257 = store_.get_group("group_257");
  EXPECT_NE(g257, nullptr);
}

TEST_F(GroupPolicyStoreEvictionTest, TouchGroupUpdatesLastSeen) {
  store_.set_time_for_test(1);

  GovApplyMsg msg1;
  msg1.group = "group_a";
  CpuPolicy cpu1;
  cpu1.max_pct = 10;
  msg1.cpu = cpu1;
  store_.upsert_group(msg1.group->c_str(), msg1);
  store_.tick();

  GovApplyMsg msg2;
  msg2.group = "group_b";
  CpuPolicy cpu2;
  cpu2.max_pct = 20;
  msg2.cpu = cpu2;
  store_.upsert_group(msg2.group->c_str(), msg2);
  store_.tick();

  store_.tick();

  GovApplyMsg msg1_touch;
  msg1_touch.group = "group_a";
  CpuPolicy cpu1_touch;
  cpu1_touch.max_pct = 11;
  msg1_touch.cpu = cpu1_touch;
  store_.upsert_group(msg1_touch.group->c_str(), msg1_touch);

  for (int i = 2; i < 257; ++i) {
    GovApplyMsg msg;
    msg.group = "group_" + std::to_string(i);
    store_.upsert_group(msg.group->c_str(), msg);
    store_.tick();
  }

  auto* ga = store_.get_group("group_a");
  EXPECT_NE(ga, nullptr);

  auto* gb = store_.get_group("group_b");
  EXPECT_EQ(gb, nullptr);
}

TEST_F(GroupPolicyStoreEvictionTest, UpdateExistingGroupDoesNotIncreaseCount) {
  store_.set_time_for_test(1);

  GovApplyMsg msg1;
  msg1.group = "group_x";
  CpuPolicy cpu1;
  cpu1.max_pct = 10;
  msg1.cpu = cpu1;
  bool ok = store_.upsert_group(msg1.group->c_str(), msg1);
  EXPECT_TRUE(ok);

  auto stats = store_.get_stats();
  EXPECT_EQ(stats.group_count, 1);

  store_.tick();

  GovApplyMsg msg2;
  msg2.group = "group_x";
  CpuPolicy cpu2;
  cpu2.max_pct = 20;
  msg2.cpu = cpu2;
  ok = store_.upsert_group(msg2.group->c_str(), msg2);
  EXPECT_TRUE(ok);

  stats = store_.get_stats();
  EXPECT_EQ(stats.group_count, 1);
  EXPECT_EQ(stats.group_evictions, 0);
}

TEST_F(GroupPolicyStoreEvictionTest, Insert8192PidsEvictsOldest) {
  store_.set_time_for_test(1);

  for (int i = 0; i < 8192; ++i) {
    bool ok = store_.map_pid_to_group(i, "group_0");
    EXPECT_TRUE(ok);
    store_.tick();
  }

  auto stats = store_.get_stats();
  EXPECT_EQ(stats.pid_group_map_count, 8192);

  bool ok = store_.map_pid_to_group(8192, "group_0");
  EXPECT_TRUE(ok);

  stats = store_.get_stats();
  EXPECT_EQ(stats.pid_group_map_count, 8192);
  EXPECT_EQ(stats.pidmap_evictions, 1);

  const char* g0 = store_.get_group_for_pid(0);
  EXPECT_EQ(g0, nullptr);

  const char* g8192 = store_.get_group_for_pid(8192);
  EXPECT_NE(g8192, nullptr);
}

TEST_F(GroupPolicyStoreEvictionTest, TouchPidUpdatesLastSeen) {
  store_.set_time_for_test(1);

  for (int i = 0; i < 3; ++i) {
    bool ok = store_.map_pid_to_group(i, "group_0");
    EXPECT_TRUE(ok);
    store_.tick();
  }

  store_.tick();

  store_.map_pid_to_group(1, "group_0");

  for (int i = 3; i < 8193; ++i) {
    store_.map_pid_to_group(i, "group_0");
    store_.tick();
  }

  const char* g1 = store_.get_group_for_pid(1);
  EXPECT_NE(g1, nullptr);

  const char* g0 = store_.get_group_for_pid(0);
  EXPECT_EQ(g0, nullptr);
}

TEST_F(GroupPolicyStoreEvictionTest, PidToGroupMapFullInsertsEvictsOldest) {
  store_.set_time_for_test(1);

  for (int i = 0; i < 8192; ++i) {
    std::string group = "group_" + std::to_string(i % 10);
    store_.map_pid_to_group(i, group.c_str());
    store_.tick();
  }

  auto stats = store_.get_stats();
  EXPECT_EQ(stats.pid_group_map_count, 8192);

  store_.map_pid_to_group(8192, "new_group");

  stats = store_.get_stats();
  EXPECT_EQ(stats.pid_group_map_count, 8192);
}

} // namespace gov
} // namespace heidi
