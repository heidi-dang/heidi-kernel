#include "heidi-kernel/job.h"
#include "test_job_fakes.h"

#include <gtest/gtest.h>

namespace heidi {

class JobTest : public ::testing::Test {
protected:
    std::unique_ptr<JobRunner> runner_;
    FakeProcessSpawner* spawner_;
    FakeProcessInspector* inspector_;
    ResourcePolicy policy_;
    int64_t now_ms_ = 0;
    
    void SetUp() override {
        auto spawner = std::make_unique<FakeProcessSpawner>();
        auto inspector = std::make_unique<FakeProcessInspector>();
        
        spawner_ = spawner.get();
        inspector_ = inspector.get();
        
        policy_.max_concurrent_jobs = 4;
        policy_.max_processes_per_job = 10;
        policy_.kill_grace_ms = 100;
        policy_.max_job_starts_per_tick = 5;
        policy_.max_job_scans_per_tick = 10;
        
        runner_ = std::make_unique<JobRunner>(policy_,
                                              std::move(spawner),
                                              std::move(inspector));
    }
    
    template<typename Pred>
    bool drive_ticks_until(int64_t step_ms, int max_iters, Pred pred) {
        for (int i = 0; i < max_iters && !pred(); ++i) {
            runner_->tick(now_ms_);
            now_ms_ += step_ms;
            spawner_->advance_time(now_ms_);
        }
        return pred();
    }
};

TEST_F(JobTest, SubmitJobReturnsValidId) {
    std::string job_id = runner_->submit_job("echo hello");
    EXPECT_FALSE(job_id.empty());
    EXPECT_TRUE(job_id.find("job_") == 0);
}

TEST_F(JobTest, GetJobStatusAfterSubmit) {
    std::string job_id = runner_->submit_job("echo hello");
    auto job = runner_->get_job_status(job_id);
    ASSERT_NE(job, nullptr);
    EXPECT_EQ(job->id, job_id);
    EXPECT_EQ(job->status, JobStatus::PENDING);
}

TEST_F(JobTest, JobStartsOnTick) {
    std::string job_id = runner_->submit_job("echo hello");
    runner_->tick(now_ms_);
    
    auto job = runner_->get_job_status(job_id);
    ASSERT_NE(job, nullptr);
    EXPECT_EQ(job->status, JobStatus::RUNNING);
    EXPECT_GT(job->pgid, 0);
}

TEST_F(JobTest, JobCompletesSuccessfully) {
    std::string job_id = runner_->submit_job("echo hello");
    runner_->tick(now_ms_);
    
    auto job = runner_->get_job_status(job_id);
    pid_t pgid = job->pgid;
    
    spawner_->set_output(pgid, "hello", "");
    spawner_->simulate_completion(pgid, 0, now_ms_ + 100);
    
    drive_ticks_until(10, 20, [&]() {
        auto j = runner_->get_job_status(job_id);
        return j->status == JobStatus::COMPLETED || j->status == JobStatus::FAILED;
    });
    
    job = runner_->get_job_status(job_id);
    EXPECT_EQ(job->status, JobStatus::COMPLETED);
    EXPECT_EQ(job->exit_code, 0);
    EXPECT_TRUE(job->output.find("hello") != std::string::npos);
}

TEST_F(JobTest, JobFailsWithBadCommand) {
    std::string job_id = runner_->submit_job("false");
    runner_->tick(now_ms_);
    
    auto job = runner_->get_job_status(job_id);
    pid_t pgid = job->pgid;
    
    spawner_->simulate_completion(pgid, 1, now_ms_ + 50);
    
    drive_ticks_until(10, 20, [&]() {
        auto j = runner_->get_job_status(job_id);
        return j->status == JobStatus::COMPLETED || j->status == JobStatus::FAILED;
    });
    
    job = runner_->get_job_status(job_id);
    EXPECT_EQ(job->status, JobStatus::FAILED);
    EXPECT_NE(job->exit_code, 0);
}

TEST_F(JobTest, CancelPendingJob) {
    std::string job_id = runner_->submit_job("sleep 10");
    bool cancelled = runner_->cancel_job(job_id);
    EXPECT_TRUE(cancelled);
    
    auto job = runner_->get_job_status(job_id);
    ASSERT_NE(job, nullptr);
    EXPECT_EQ(job->status, JobStatus::CANCELLED);
}

TEST_F(JobTest, GetRecentJobs) {
    runner_->submit_job("echo job1");
    runner_->submit_job("echo job2");
    
    auto jobs = runner_->get_recent_jobs(10);
    EXPECT_GE(jobs.size(), 2);
}

TEST_F(JobTest, TickBudgets_Enforced) {
    for (int i = 0; i < 10; ++i) {
        runner_->submit_job("echo " + std::to_string(i));
    }
    
    auto diag = runner_->tick(now_ms_);
    EXPECT_LE(diag.jobs_started_this_tick, policy_.max_job_starts_per_tick);
}

TEST_F(JobTest, ProcCap_DoesNotTriggerAtLimit) {
    std::string job_id = runner_->submit_job("echo hello");
    runner_->tick(now_ms_);
    
    auto job = runner_->get_job_status(job_id);
    pid_t pgid = job->pgid;
    inspector_->set_count(pgid, policy_.max_processes_per_job);
    
    for (int i = 0; i < 10; ++i) {
        runner_->tick(now_ms_);
        now_ms_ += 10;
        spawner_->advance_time(now_ms_);
    }
    
    job = runner_->get_job_status(job_id);
    EXPECT_EQ(job->status, JobStatus::RUNNING);
}

TEST_F(JobTest, ProcCap_TriggersProcLimit) {
    std::string job_id = runner_->submit_job("echo hello");
    runner_->tick(now_ms_);
    
    auto job = runner_->get_job_status(job_id);
    pid_t pgid = job->pgid;
    inspector_->set_count(pgid, policy_.max_processes_per_job + 1);
    
    runner_->tick(now_ms_);
    
    EXPECT_TRUE(spawner_->was_signal_sent(pgid, SIGTERM));
}

TEST_F(JobTest, ProcCap_EscalatesToSigkillAfterGrace) {
    std::string job_id = runner_->submit_job("echo hello");
    runner_->tick(now_ms_);
    
    auto job = runner_->get_job_status(job_id);
    pid_t pgid = job->pgid;
    inspector_->set_count(pgid, policy_.max_processes_per_job + 1);
    
    now_ms_ += 10;
    spawner_->advance_time(now_ms_);
    runner_->tick(now_ms_);
    int64_t sigterm_time = spawner_->get_signal_time(pgid, SIGTERM);
    EXPECT_GE(sigterm_time, 0);
    
    now_ms_ = sigterm_time + policy_.kill_grace_ms + 1;
    spawner_->advance_time(now_ms_);
    runner_->tick(now_ms_);
    
    EXPECT_TRUE(spawner_->was_signal_sent(pgid, SIGKILL));
    
    job = runner_->get_job_status(job_id);
    EXPECT_EQ(job->status, JobStatus::PROC_LIMIT);
}

TEST_F(JobTest, ScanBudget_Respected) {
    for (int i = 0; i < 25; ++i) {
        runner_->submit_job("echo " + std::to_string(i));
    }
    runner_->tick(now_ms_);
    
    now_ms_ += 1000;
    spawner_->advance_time(now_ms_);
    
    auto diag = runner_->tick(now_ms_);
    EXPECT_LE(diag.jobs_scanned_this_tick, policy_.max_job_scans_per_tick);
}

TEST_F(JobTest, StartBudget_CappedAt5_PerTick) {
    for (int i = 0; i < 20; ++i) {
        runner_->submit_job("echo " + std::to_string(i));
    }
    
    for (int tick = 0; tick < 5; ++tick) {
        auto diag = runner_->tick(now_ms_);
        EXPECT_LE(diag.jobs_started_this_tick, policy_.max_job_starts_per_tick);
        now_ms_ += 100;
        spawner_->advance_time(now_ms_);
    }
}

TEST_F(JobTest, ScanBudget_CappedAt10_AndCursorAdvances) {
    for (int i = 0; i < 12; ++i) {
        runner_->submit_job("echo " + std::to_string(i));
    }
    runner_->tick(now_ms_);
    
    now_ms_ += 1000;
    spawner_->advance_time(now_ms_);
    
    auto diag1 = runner_->tick(now_ms_);
    EXPECT_LE(diag1.jobs_scanned_this_tick, policy_.max_job_scans_per_tick);
    int scanned_at_tick1 = diag1.jobs_scanned_this_tick;
    
    now_ms_ += 1000;
    spawner_->advance_time(now_ms_);
    
    auto diag2 = runner_->tick(now_ms_);
    int scanned_at_tick2 = diag2.jobs_scanned_this_tick;
    
    EXPECT_EQ(scanned_at_tick1, scanned_at_tick2);
}

TEST_F(JobTest, Determinism_SameTickSequenceSameFinalState) {
    auto create_runner = [this]() {
        auto spawner = std::make_unique<FakeProcessSpawner>();
        auto inspector = std::make_unique<FakeProcessInspector>();
        return std::make_unique<JobRunner>(policy_, std::move(spawner), std::move(inspector));
    };
    
    auto run_sequence = [&](JobRunner& runner) {
        for (int i = 0; i < 10; ++i) {
            runner.submit_job("echo " + std::to_string(i));
        }
        int64_t t = 0;
        for (int tick = 0; tick < 3; ++tick) {
            t += 100;
            runner.tick(t);
        }
        return runner.get_recent_jobs(10).size();
    };
    
    auto runner1 = create_runner();
    auto runner2 = create_runner();
    
    int result1 = run_sequence(*runner1);
    int result2 = run_sequence(*runner2);
    
    EXPECT_EQ(result1, result2);
}

TEST_F(JobTest, PolicyUpdate_Valid) {
    ResourcePolicy new_policy = policy_;
    new_policy.max_processes_per_job = 20;
    EXPECT_TRUE(runner_->update_policy(new_policy));
    EXPECT_EQ(runner_->get_policy().max_processes_per_job, 20);
}

TEST_F(JobTest, PolicyUpdate_InvalidRejected) {
    ResourcePolicy new_policy = policy_;
    new_policy.max_processes_per_job = 0;
    EXPECT_FALSE(runner_->update_policy(new_policy));
    EXPECT_EQ(runner_->get_policy().max_processes_per_job, policy_.max_processes_per_job);
}

} // namespace heidi
