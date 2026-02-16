#include "heidi-kernel/daemon.h"
#include "heidi-kernel/ipc.h"

#include <csignal>
#include <iostream>

namespace heidi {

static volatile bool g_running = true;

void signal_handler(int sig) {
    g_running = false;
}

Daemon::Daemon(const std::string& socket_path) : socket_path_(socket_path) {
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
}

Daemon::~Daemon() = default;

void Daemon::run() {
    running_ = true;
    UnixSocketServer server(socket_path_);
    
    std::cout << "Daemon started on " << socket_path_ << std::endl;
    
    while (running_ && g_running) {
        server.serve_forever();
    }
    
    server.stop();
    std::cout << "Daemon stopped" << std::endl;
}

void Daemon::stop() {
    running_ = false;
}

} // namespace heidi