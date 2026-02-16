#include "heidi-kernel/policy_store.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <cerrno>
#include <unistd.h>
#include <fcntl.h>
#include <algorithm>

namespace heidi {

PolicyStore::PolicyStore(std::string path) : path_(std::move(path)) {}

GovernorPolicy PolicyStore::load() {
    GovernorPolicy policy = GovernorPolicy(); // defaults

    std::ifstream file(path_);
    if (!file) {
        std::cerr << "Policy file not found, using defaults\n";
        return policy;
    }

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    // Simple JSON parser for the policy
    // Assume format: {"key":value, ...}
    std::istringstream iss(content);
    std::string token;
    while (std::getline(iss, token, ',')) {
        // Trim
        token.erase(token.begin(), std::find_if(token.begin(), token.end(), [](unsigned char ch) { return !std::isspace(ch) && ch != '{' && ch != '}'; }));
        token.erase(std::find_if(token.rbegin(), token.rend(), [](unsigned char ch) { return !std::isspace(ch) && ch != '{' && ch != '}'; }).base(), token.end());

        size_t colon = token.find(':');
        if (colon != std::string::npos) {
            std::string key = token.substr(0, colon);
            std::string value = token.substr(colon + 1);

            // Trim quotes from key
            if (key.front() == '"') key = key.substr(1);
            if (key.back() == '"') key = key.substr(0, key.size() - 1);

            // Parse value
            if (key == "max_running_jobs") {
                policy.max_running_jobs = std::stoi(value);
            } else if (key == "max_queue_depth") {
                policy.max_queue_depth = std::stoi(value);
            } else if (key == "cpu_high_watermark_pct") {
                policy.cpu_high_watermark_pct = std::stod(value);
            } else if (key == "mem_high_watermark_pct") {
                policy.mem_high_watermark_pct = std::stod(value);
            } else if (key == "cooldown_ms") {
                policy.cooldown_ms = std::stoull(value);
            } else if (key == "min_start_gap_ms") {
                policy.min_start_gap_ms = std::stoull(value);
            }
        }
    }

    // Validate ranges
    if (policy.max_running_jobs < 1 || policy.max_running_jobs > 1000) {
        std::cerr << "Invalid max_running_jobs, using default\n";
        policy.max_running_jobs = 10;
    }
    if (policy.max_queue_depth < 1 || policy.max_queue_depth > 10000) {
        std::cerr << "Invalid max_queue_depth, using default\n";
        policy.max_queue_depth = 100;
    }
    // Add more validations

    return policy;
}

void PolicyStore::save(const GovernorPolicy& policy) {
    std::string temp_path = path_ + ".tmp";

    std::ofstream file(temp_path);
    if (!file) {
        throw std::runtime_error("Failed to open temp policy file");
    }

    file << "{\n";
    file << "  \"max_running_jobs\": " << policy.max_running_jobs << ",\n";
    file << "  \"max_queue_depth\": " << policy.max_queue_depth << ",\n";
    file << "  \"cpu_high_watermark_pct\": " << policy.cpu_high_watermark_pct << ",\n";
    file << "  \"mem_high_watermark_pct\": " << policy.mem_high_watermark_pct << ",\n";
    file << "  \"cooldown_ms\": " << policy.cooldown_ms << ",\n";
    file << "  \"min_start_gap_ms\": " << policy.min_start_gap_ms << "\n";
    file << "}\n";

    file.close();

    // fsync
    int fd = open(temp_path.c_str(), O_RDONLY);
    if (fd >= 0) {
        fsync(fd);
        close(fd);
    }

    // rename
    if (rename(temp_path.c_str(), path_.c_str()) != 0) {
        throw std::runtime_error("Failed to rename policy file");
    }
}

} // namespace heidi