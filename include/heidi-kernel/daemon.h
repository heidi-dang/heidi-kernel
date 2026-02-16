#pragma once

#include <string>

namespace heidi {

class Daemon {
public:
    Daemon(const std::string& socket_path);
    ~Daemon();

    void run();
    void stop();

private:
    std::string socket_path_;
    bool running_ = false;
};

} // namespace heidi