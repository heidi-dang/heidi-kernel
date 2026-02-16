#include <chrono>
#include <csignal>
#include <iostream>
#include <thread>

#include "heidi-kernel/config.h"
#include "heidi-kernel/event_loop.h"
#include "heidi-kernel/logger.h"
#include "heidi-kernel/result.h"

namespace {

std::sig_atomic_t g_signal_received = 0;

void signal_handler(int signal) {
    g_signal_received = signal;
}

} // namespace

int main(int argc, char* argv[]) {
    auto config_result = heidi::ConfigParser::parse(argc, argv);
    if (!config_result) {
        std::cerr << "Error: " << config_result.error().message << "\n";
        return static_cast<int>(config_result.error().code);
    }

    auto& config = config_result.value();

    if (config.show_version) {
        std::cout << "heidi-kernel " << heidi::ConfigParser::version() << "\n";
        return 0;
    }

    if (config.show_help) {
        std::cout << "Usage: heidi-kernel [OPTIONS]\n"
                  << "Options:\n"
                  << "  --config <path>   Config file path\n"
                  << "  --log-level <level>  Log level (debug, info, warn, error)\n"
                  << "  --version, -v    Show version\n"
                  << "  --help, -h       Show this help\n";
        return 0;
    }

    heidi::Logger logger;
    if (config.log_level == "debug") {
        logger.set_level(heidi::LogLevel::Debug);
    } else if (config.log_level == "warn") {
        logger.set_level(heidi::LogLevel::Warn);
    } else if (config.log_level == "error") {
        logger.set_level(heidi::LogLevel::Error);
    } else {
        logger.set_level(heidi::LogLevel::Info);
    }

    logger.info("heidi-kernel starting");
    logger.info("version: " + std::string(heidi::ConfigParser::version()));

    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    heidi::EventLoop loop{std::chrono::milliseconds{100}};
    int tick_count = 0;

    loop.set_tick_callback([&logger, &tick_count](std::chrono::milliseconds) {
        ++tick_count;
        if (tick_count % 10 == 0) {
            logger.debug("tick " + std::to_string(tick_count));
        }
    });

    logger.info("event loop starting");
    loop.run();

    while (loop.is_running() && g_signal_received == 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds{50});
    }

    if (g_signal_received != 0) {
        logger.warn("shutdown requested (signal " + std::to_string(g_signal_received) + ")");
    }

    logger.info("stopping event loop");
    loop.request_stop();

    while (loop.is_running()) {
        std::this_thread::sleep_for(std::chrono::milliseconds{10});
    }

    logger.info("heidi-kernel stopped");
    logger.info("total ticks: " + std::to_string(tick_count));

    return g_signal_received == SIGINT ? 130 : 0;
}
