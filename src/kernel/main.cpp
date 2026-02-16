#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <string_view>
#include <thread>
#include <unistd.h>

#include "heidi-kernel/event_loop.h"
#include "heidi-kernel/logger.h"
#include "heidi-kernel/status.h"

namespace {

std::sig_atomic_t g_signal_received = 0;

void signal_handler(int signal) {
    g_signal_received = signal;
}

std::string get_socket_path() {
    const char* xdg_runtime = getenv("XDG_RUNTIME_DIR");
    if (xdg_runtime && xdg_runtime[0]) {
        return std::string(xdg_runtime) + "/heidi-kernel.sock";
    }
    return "/tmp/heidi-kernel.sock";
}

} // namespace

int main(int, char*[]) {
    std::string socket_path = get_socket_path();

    heidi::Logger logger;
    logger.info("heidi-kernel starting");
    logger.info("socket: " + socket_path);

    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    heidi::StatusSocket status_socket(socket_path);
    status_socket.bind();

    logger.info("status socket bound");

    std::thread status_thread([&status_socket]() {
        status_socket.serve_forever();
    });

    heidi::EventLoop loop{std::chrono::milliseconds{100}};
    int tick_count = 0;

    loop.set_tick_callback([&logger, &tick_count](std::chrono::milliseconds) {
        ++tick_count;
        if (tick_count % 100 == 0) {
            logger.debug("tick " + std::to_string(tick_count));
        }
    });

    loop.run();
    logger.info("event loop running");

    while (!g_signal_received) {
        std::this_thread::sleep_for(std::chrono::milliseconds{100});
    }

    logger.info("shutdown requested");
    status_socket.set_stop();
    loop.request_stop();

    while (loop.is_running()) {
        std::this_thread::sleep_for(std::chrono::milliseconds{10});
    }

    if (status_thread.joinable()) {
        status_thread.join();
    }

    logger.info("heidi-kernel stopped");
    return 0;
}
