#include "heidi-kernel/job.h"

#include <gtest/gtest.h>
#include <chrono>
#include <thread>

namespace heidi {

class JobTest : public ::testing::Test {
protected:
    void SetUp() override {
        job_runner_ = new JobRunner(2); // Small concurrency for testing
        job_runner_->start();
    }

    void TearDown() override {
        job_runner_->stop();
        delete job_runner_;
    }

    JobRunner* job_runner_;
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
    EXPECT_EQ(job->id, job_id);
    EXPECT_TRUE(job->status == JobStatus::PENDING || job->status == JobStatus::RUNNING);
}

TEST_F(JobTest, JobCompletesSuccessfully) {
    std::string job_id = job_runner_->submit_job("echo hello");
    
    // Wait for job to complete
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    auto job = job_runner_->get_job_status(job_id);
    ASSERT_NE(job, nullptr);
    EXPECT_EQ(job->status, JobStatus::COMPLETED);
    EXPECT_EQ(job->exit_code, 0);
    EXPECT_TRUE(job->output.find("hello") != std::string::npos);
}

TEST_F(JobTest, JobFailsWithBadCommand) {
    std::string job_id = job_runner_->submit_job("false");
    
    // Wait for job to complete
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    auto job = job_runner_->get_job_status(job_id);
    ASSERT_NE(job, nullptr);
    EXPECT_EQ(job->status, JobStatus::FAILED);
    EXPECT_NE(job->exit_code, 0);
}

TEST_F(JobTest, CancelPendingJob) {
    std::string job_id = job_runner_->submit_job("sleep 10");
    
    bool cancelled = job_runner_->cancel_job(job_id);
    EXPECT_TRUE(cancelled);
    
    auto job = job_runner_->get_job_status(job_id);
    ASSERT_NE(job, nullptr);
    EXPECT_EQ(job->status, JobStatus::CANCELLED);
}

TEST_F(JobTest, GetRecentJobs) {
    std::string job_id1 = job_runner_->submit_job("echo job1");
    std::string job_id2 = job_runner_->submit_job("echo job2");
    
    auto jobs = job_runner_->get_recent_jobs(10);
    EXPECT_GE(jobs.size(), 2);
    
    // Check that our jobs are in the list
    bool found1 = false, found2 = false;
    for (const auto& job : jobs) {
        if (job->id == job_id1) found1 = true;
        if (job->id == job_id2) found2 = true;
    }
    EXPECT_TRUE(found1);
    EXPECT_TRUE(found2);
}

} // namespace heidi