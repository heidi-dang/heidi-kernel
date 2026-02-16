#include "heidi-kernel/metrics.h"
#include <gtest/gtest.h>

namespace heidi {
namespace {

TEST(MetricsSamplerTest, SampleCpu) {
    MetricsSampler sampler;
    SystemMetrics metrics = sampler.sample();
    
    EXPECT_GE(metrics.cpu_usage_percent, 0.0);
    EXPECT_LE(metrics.cpu_usage_percent, 100.0);
}

TEST(MetricsSamplerTest, SampleMem) {
    MetricsSampler sampler;
    SystemMetrics metrics = sampler.sample();
    
    EXPECT_GT(metrics.mem.total, 0);
    EXPECT_GE(metrics.mem.free, 0);
    EXPECT_LE(metrics.mem.free, metrics.mem.total);
}

} // namespace
} // namespace heidi