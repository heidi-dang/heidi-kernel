#include "heidi-kernel/policy_store.h"

#include <gtest/gtest.h>
#include <fstream>
#include <filesystem>

namespace heidi {

class PolicyStoreTest : public ::testing::Test {
protected:
    void SetUp() override {
        temp_path_ = "/tmp/heidi_test_policy.json";
        // Remove if exists
        std::filesystem::remove(temp_path_);
    }

    void TearDown() override {
        std::filesystem::remove(temp_path_);
    }

    std::string temp_path_;
};

TEST_F(PolicyStoreTest, LoadMissingFile) {
    PolicyStore store(temp_path_);
    GovernorPolicy policy = store.load();
    EXPECT_EQ(policy.max_running_jobs, 10);
    EXPECT_EQ(policy.max_queue_depth, 100);
}

TEST_F(PolicyStoreTest, SaveAndLoad) {
    PolicyStore store(temp_path_);

    GovernorPolicy policy;
    policy.max_running_jobs = 20;
    policy.max_queue_depth = 200;
    policy.cpu_high_watermark_pct = 90.0;

    store.save(policy);

    GovernorPolicy loaded = store.load();
    EXPECT_EQ(loaded.max_running_jobs, 20);
    EXPECT_EQ(loaded.max_queue_depth, 200);
    EXPECT_EQ(loaded.cpu_high_watermark_pct, 90.0);
}

TEST_F(PolicyStoreTest, InvalidJsonFallback) {
    // Write invalid JSON
    std::ofstream file(temp_path_);
    file << "invalid json";
    file.close();

    PolicyStore store(temp_path_);
    GovernorPolicy policy = store.load();
    EXPECT_EQ(policy.max_running_jobs, 10); // defaults
}

TEST_F(PolicyStoreTest, InvalidRangeFallback) {
    // Write JSON with invalid range
    std::ofstream file(temp_path_);
    file << "{\"max_running_jobs\": -1, \"max_queue_depth\": 100}";
    file.close();

    PolicyStore store(temp_path_);
    GovernorPolicy policy = store.load();
    EXPECT_EQ(policy.max_running_jobs, 10); // fallback to default
    EXPECT_EQ(policy.max_queue_depth, 100); // valid
}

} // namespace heidi