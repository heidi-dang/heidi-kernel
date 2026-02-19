#include "heidi-kernel/job.h"
#include "heidi-kernel/metrics.h"
#include "heidi-kernel/process_inspector.h"

#include <gtest/gtest.h>
#include <chrono>
#include <thread>
#include <unordered_map>

namespace heidi {

// Fake implementations for testing
class FakeProcessSpawner : public IProcessSpawner {
public:
    bool spawn_job(Job& job, int* stdout_fd, int* stderr_fd) override {
        // Simulate successful spawn
        job.process_group = next_pgid_++;
        *stdout_fd = -1; // No real pipes in fake
        *stderr_fd = -1;
        return true;
    }
private:
    pid_t next_pgid_ = 1000;
};

class FakeProcessInspector : public IProcessInspector {
public:
    void set_process_count(pid_t pgid, int count) {
        counts_[pgid] = count;
    }

    int count_processes_in_pgid(pid_t pgid) override {
        auto it = counts_.find(pgid);
        return it != counts_.end() ? it->second : 0;
    }
private:
    std::unordered_map<pid_t, int> counts_;
};

class JobTest : public ::testing::Test {
protected:
    void SetUp() override {
        spawner_ = new FakeProcessSpawner();
        inspector_ = new FakeProcessInspector();
        job_runner_ = new heidi::JobRunner(20, spawner_, inspector_); // Higher concurrency for budget tests
        job_runner_->start();
    }

    void TearDown() override {
        job_runner_->stop();
        delete job_runner_;
        delete spawner_;
        delete inspector_;
    }

    heidi::JobRunner* job_runner_;
    FakeProcessSpawner* spawner_;
    FakeProcessInspector* inspector_;
};

TEST_F(JobTest, SubmitJobReturnsValidId) {
    std::string job_id = job_runner_->submit_job("echo hello");
    EXPECT_FALSE(job_id.empty());
    EXPECT_TRUE(job_id.find("job_") == 0);
}

TEST_F(JobTest, GetJobStatusAfterSubmit) {
    std::string job_id = job_runner_->submit_job("echo hello");
    auto job = job_runner_->get_job_status(job_id);
    ASSERT_NE(job, nullptr);
    EXPECT_TRUE(job->status == heidi::JobStatus::QUEUED || job->status == heidi::JobStatus::RUNNING || job->status == heidi::JobStatus::STARTING);
}

TEST_F(JobTest, JobCompletesSuccessfully) {
    std::string job_id = job_runner_->submit_job("echo hello");

    auto job = job_runner_->get_job_status(job_id);
    EXPECT_EQ(job->status, heidi::JobStatus::QUEUED);

    // Start the job
    heidi::SystemMetrics metrics{10.0, {1000, 100, 400}, 0};
    uint64_t now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    job_runner_->tick(now_ms, metrics);

    EXPECT_EQ(job_runner_->get_last_tick_diagnostics().last_decision, heidi::GovernorDecision::START_NOW);

    job = job_runner_->get_job_status(job_id);
    ASSERT_NE(job, nullptr);
    EXPECT_EQ(job->status, heidi::JobStatus::RUNNING);
}

TEST_F(JobTest, ProcCap_TriggersProcLimit) {
    heidi::JobLimits limits;
    limits.max_child_processes = 10;

    std::string job_id = job_runner_->submit_job("sleep 10", limits);

    // Start the job
    heidi::SystemMetrics metrics{10.0, {1000, 100, 400}, 0};
    uint64_t now_ms = 0;
    job_runner_->tick(now_ms, metrics);

    auto job = job_runner_->get_job_status(job_id);
    ASSERT_NE(job, nullptr);
    EXPECT_EQ(job->status, heidi::JobStatus::RUNNING);

    // Set process count over limit
    inspector_->set_process_count(job->process_group, 11);

    // Tick to enforce
    now_ms = 1000;
    job_runner_->tick(now_ms, metrics, 5, 10);

    job = job_runner_->get_job_status(job_id);
    ASSERT_NE(job, nullptr);
    EXPECT_EQ(job->status, heidi::JobStatus::PROC_LIMIT);
    EXPECT_EQ(job->ended_at_ms, 1000);
}

TEST_F(JobTest, ProcCap_DoesNotTriggerAtLimit) {
    heidi::JobLimits limits;
    limits.max_child_processes = 10;

    std::string job_id = job_runner_->submit_job("sleep 10", limits);

    // Start the job
    heidi::SystemMetrics metrics{10.0, {1000, 100, 400}, 0};
    uint64_t now_ms = 0;
    job_runner_->tick(now_ms, metrics);

    auto job = job_runner_->get_job_status(job_id);
    ASSERT_NE(job, nullptr);
    EXPECT_EQ(job->status, heidi::JobStatus::RUNNING);

    // Set process count at limit
    inspector_->set_process_count(job->process_group, 10);

    // Tick to scan
    now_ms = 1000;
    job_runner_->tick(now_ms, metrics, 5, 10);

    job = job_runner_->get_job_status(job_id);
    ASSERT_NE(job, nullptr);
    EXPECT_EQ(job->status, heidi::JobStatus::RUNNING); // Still running
}

TEST_F(JobTest, Determinism_RepeatedTicksSameInputsSameOutputs) {
    // Submit a job
    std::string job_id = job_runner_->submit_job("echo hello");

    heidi::SystemMetrics metrics{10.0, {1000, 100, 400}, 0};
    uint64_t now_ms = 1000;

    // Tick once
    job_runner_->tick(now_ms, metrics);

    // Record state
    auto jobs1 = job_runner_->get_recent_jobs(10);
    auto diag1 = job_runner_->get_last_tick_diagnostics();

    // Reset and repeat
    job_runner_->stop();
    delete job_runner_;
    job_runner_ = new heidi::JobRunner(20, spawner_, inspector_);
    job_runner_->start();

    std::string job_id2 = job_runner_->submit_job("echo hello");
    job_runner_->tick(now_ms, metrics);

    auto jobs2 = job_runner_->get_recent_jobs(10);
    auto diag2 = job_runner_->get_last_tick_diagnostics();

    // Compare
    ASSERT_EQ(jobs1.size(), jobs2.size());
    for (size_t i = 0; i < jobs1.size(); ++i) {
        EXPECT_EQ(jobs1[i]->status, jobs2[i]->status);
        EXPECT_EQ(jobs1[i]->ended_at_ms, jobs2[i]->ended_at_ms);
    }
    EXPECT_EQ(diag1.last_decision, diag2.last_decision);
    EXPECT_EQ(diag1.last_tick_now_ms, diag2.last_tick_now_ms);
}

} // namespace heidi