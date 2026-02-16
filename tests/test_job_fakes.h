#pragma once

#include "heidi-kernel/job.h"
#include "heidi-kernel/process_inspector.h"

#include <unordered_map>
#include <vector>
#include <utility>

namespace heidi {

// Fake ProcessInspector for testing
class FakeProcessInspector : public IProcessInspector {
public:
    // Configure process counts per pgid
    std::unordered_map<pid_t, int> process_counts;
    int default_count = 0;
    
    int count_processes_in_pgid(pid_t pgid) override {
        auto it = process_counts.find(pgid);
        if (it != process_counts.end()) {
            return it->second;
        }
        return default_count;
    }
    
    void set_count(pid_t pgid, int count) {
        process_counts[pgid] = count;
    }
};

// Fake ProcessSpawner for testing
class FakeProcessSpawner : public IProcessSpawner {
public:
    struct SpawnedProcess {
        std::string command;
        pid_t pgid;
        bool completed = false;
        int exit_code = 0;
        int64_t completion_time_ms = -1;  // -1 means not scheduled
        std::string stdout_data;
        std::string stderr_data;
    };
    
    struct SignalRecord {
        pid_t pgid;
        int signal;
        int64_t time_ms;
    };
    
    // Configurable behavior
    pid_t next_pgid = 1000;
    std::unordered_map<pid_t, SpawnedProcess> processes;
    std::vector<SignalRecord> signals_sent;
    
    // For driving test progression
    int64_t current_time_ms = 0;
    
    pid_t spawn_process_group(const std::string& command) override {
        pid_t pgid = next_pgid++;
        processes[pgid] = {command, pgid, false, 0, -1, "", ""};
        return pgid;
    }
    
    bool signal_pgid(pid_t pgid, int signal) override {
        signals_sent.push_back({pgid, signal, current_time_ms});
        return true;
    }
    
    bool check_completion(pid_t pgid, int& exit_code) override {
        auto it = processes.find(pgid);
        if (it == processes.end()) return false;
        
        if (it->second.completed || 
            (it->second.completion_time_ms >= 0 && current_time_ms >= it->second.completion_time_ms)) {
            it->second.completed = true;
            exit_code = it->second.exit_code;
            return true;
        }
        return false;
    }
    
    void collect_output(pid_t pgid, std::string& stdout_out, std::string& stderr_out) override {
        auto it = processes.find(pgid);
        if (it != processes.end()) {
            stdout_out = it->second.stdout_data;
            stderr_out = it->second.stderr_data;
        }
    }
    
    // Test helpers
    void simulate_completion(pid_t pgid, int exit_code, int64_t at_time_ms = -1) {
        auto it = processes.find(pgid);
        if (it != processes.end()) {
            it->second.completion_time_ms = (at_time_ms >= 0) ? at_time_ms : current_time_ms;
            it->second.exit_code = exit_code;
        }
    }
    
    void set_output(pid_t pgid, const std::string& stdout_data, const std::string& stderr_data) {
        auto it = processes.find(pgid);
        if (it != processes.end()) {
            it->second.stdout_data = stdout_data;
            it->second.stderr_data = stderr_data;
        }
    }
    
    bool was_signal_sent(pid_t pgid, int signal) const {
        for (const auto& rec : signals_sent) {
            if (rec.pgid == pgid && rec.signal == signal) {
                return true;
            }
        }
        return false;
    }
    
    int64_t get_signal_time(pid_t pgid, int signal) const {
        for (const auto& rec : signals_sent) {
            if (rec.pgid == pgid && rec.signal == signal) {
                return rec.time_ms;
            }
        }
        return -1;
    }
    
    void advance_time(int64_t time_ms) {
        current_time_ms = time_ms;
    }
    
    void reset() {
        next_pgid = 1000;
        processes.clear();
        signals_sent.clear();
        current_time_ms = 0;
    }
};

// Test helper for driving ticks
template<typename Predicate>
bool drive_ticks(JobRunner& runner, int64_t& now_ms, int64_t step_ms, int max_iters, Predicate pred) {
    for (int i = 0; i < max_iters; ++i) {
        runner.tick(now_ms);
        if (pred()) {
            return true;
        }
        now_ms += step_ms;
    }
    return false;
}

} // namespace heidi
