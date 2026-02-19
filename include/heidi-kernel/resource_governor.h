#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <optional>

namespace heidi {

enum class GovernorDecision {
    START_NOW,
    HOLD_QUEUE,
    REJECT_QUEUE_FULL
};

enum class BlockReason {
    NONE,
    CPU_HIGH,
    MEM_HIGH,
    QUEUE_FULL,
    RUNNING_LIMIT
};

struct GovernorResult {
    GovernorDecision decision;
    BlockReason reason;
    uint64_t retry_after_ms;
};

struct GovernorPolicy {
    int max_running_jobs = 10;
    int max_queue_depth = 100;
    double cpu_high_watermark_pct = 85.0;
    double mem_high_watermark_pct = 90.0;
    uint64_t cooldown_ms = 1000;
    uint64_t min_start_gap_ms = 100;
};

struct PolicyValidationError {
    std::string field;
    std::string message;
};

struct PolicyUpdateResult {
    bool success;
    GovernorPolicy effective_policy;
    std::vector<PolicyValidationError> errors;
};

class ResourceGovernor {
public:
    explicit ResourceGovernor(const GovernorPolicy& policy = GovernorPolicy());

    GovernorResult decide(double cpu_pct, double mem_pct, int running_jobs, int queued_jobs) const;

    void update_policy(const GovernorPolicy& policy);
    const GovernorPolicy& get_policy() const;

    PolicyUpdateResult validate_and_update(const GovernorPolicy& policy);

private:
    GovernorPolicy policy_;
};

} // namespace heidi