#include "heidi-kernel/config.h"
#include "heidi-kernel/event_loop.h"
#include "heidi-kernel/logger.h"
#include "heidi-kernel/status.h"

#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <string_view>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>
<<<<<<< HEAD

<<<<<<< HEAD
=======
#include "heidi-kernel/config.h"
#include "heidi-kernel/event_loop.h"
#include "heidi-kernel/logger.h"
#include "heidi-kernel/status.h"
    =======
>>>>>>> 88f1ce3 (chore(kernel): IPC contract polish + runtime socket path)

    >>>>>>> 3bdbab5(chore(kernel)
                    : IPC contract polish + runtime socket path) namespace {

  std::sig_atomic_t g_signal_received = 0;

<<<<<<< HEAD
  void signal_handler(int signal) {
    g_signal_received = signal;
  }
=======
  void signal_handler(int signal) {
    g_signal_received = signal;
  }
>>>>>>> 88f1ce3 (chore(kernel): IPC contract polish + runtime socket path)

} // namespace

int main(int argc, char* argv[]) {
  heidi::Config config;
  auto config_res = heidi::ConfigParser::parse(argc, argv);
  if (config_res.ok()) {
    config = config_res.value();
  }

  if (config.show_help) {
    std::printf("Usage: heidi-kernel [options]\n");
    std::printf("Options:\n");
    std::printf("  -h, --help            Show this help message\n");
    std::printf("  -v, --version         Show version information\n");
    std::printf("  --log-level <level>   Set log level (debug, info, warn, error)\n");
    std::printf("  --socket-path <path>  Set Unix domain socket path\n");
    return 0;
  }

  if (config.show_version) {
    std::printf("heidi-kernel version %s\n", std::string(heidi::ConfigParser::version()).c_str());
    return 0;
  }

  std::string socket_path;
  if (!config.socket_path.empty()) {
    socket_path = std::string(config.socket_path);
  } else if (const char* xdg_runtime = std::getenv("XDG_RUNTIME_DIR")) {
    socket_path = std::string(xdg_runtime) + "/heidi-kernel.sock";
  } else {
    socket_path = "/run/heidi-kernel/heidi-kernel.sock";
  }

  // Ensure directory exists with safe permissions
  std::filesystem::path p(socket_path);
  std::filesystem::path dir = p.parent_path();
  if (!dir.empty() && !std::filesystem::exists(dir)) {
    try {
      std::filesystem::create_directories(dir);
      chmod(dir.c_str(), 0755);
    } catch (const std::exception& e) {
      std::fprintf(stderr, "Failed to create socket directory: %s\n", e.what());
    }
  }

  heidi::Logger logger;
  logger.info("heidi-kernel starting");
  logger.info("socket: " + socket_path);

  std::signal(SIGINT, signal_handler);
  std::signal(SIGTERM, signal_handler);

  heidi::StatusSocket status_socket(socket_path);
  status_socket.bind();

  logger.info("status socket bound");

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
  loop.request_stop();

  while (loop.is_running()) {
    std::this_thread::sleep_for(std::chrono::milliseconds{10});
  }

  logger.info("heidi-kernel stopped");
  return 0;
}
