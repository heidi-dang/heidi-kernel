#pragma once

#include <chrono>
#include <cstdint>
#include <string_view>

namespace heidi {

struct KernelStatus {
    std::string_view version;
    int pid;
    std::chrono::milliseconds uptime_ms;
    uint64_t rss_kb;
    int threads;
    int queue_depth;
};

class StatusSocket {
public:
    explicit StatusSocket(std::string_view socket_path);
    ~StatusSocket();

    void bind();
    void close();

    void serve_forever();

private:
    void handle_client(int client_fd);
    std::string format_status() const;

    std::string socket_path_;
    int server_fd_ = -1;
    KernelStatus status_;
};

}
