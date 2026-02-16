#include "heidi-kernel/http.h"

#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <poll.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <thread>
#include <unistd.h>

namespace {

volatile std::sig_atomic_t g_running = 1;
std::mutex g_status_mutex;
std::string g_kernel_status = "{\"error\":\"not connected\"}";

void signal_handler(int) {
  g_running = 0;
}

std::string query_kernel(const std::string& cmd, const std::string& body = "") {
  const char* xdg_runtime = getenv("XDG_RUNTIME_DIR");
  std::string socket_path = xdg_runtime && xdg_runtime[0]
                                ? std::string(xdg_runtime) + "/heidi-kernel.sock"
                                : "/tmp/heidi-kernel.sock";

  int fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (fd < 0) {
    return "{\"error\":\"socket_failed\"}";
  }

  struct sockaddr_un addr;
  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, socket_path.c_str(), sizeof(addr.sun_path) - 1);

  if (connect(fd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) {
    close(fd);
    return "{\"error\":\"kernel_not_running\"}";
  }

  std::string request = cmd;
  if (!body.empty()) {
    request += " " + body;
  }
  request += "\n";
  ssize_t w = write(fd, request.c_str(), request.size());
  if (w < 0) {
    close(fd);
    return "{\"error\":\"write_failed\"}";
  }

  struct pollfd pfd = {fd, POLLIN, 0};
  int r = poll(&pfd, 1, 1000);
  if (r <= 0) {
    close(fd);
    return "{\"error\":\"timeout\"}";
  }

  char buf[4096];
  ssize_t n = read(fd, buf, sizeof(buf) - 1);
  close(fd);

  if (n <= 0) {
    return "{\"error\":\"no_response\"}";
  }
  buf[n] = '\0';
  return std::string(buf, n);
}

std::string parse_kv_to_json(const std::string& raw) {
  std::istringstream iss(raw);
  std::string line;
  std::getline(iss, line); // skip command line
  std::ostringstream json;
  json << "{";
  bool first = true;
  while (std::getline(iss, line)) {
    size_t colon = line.find(':');
    if (colon != std::string::npos) {
      std::string key = line.substr(0, colon);
      std::string value = line.substr(colon + 1);
      // Trim spaces
      key.erase(key.begin(), std::find_if(key.begin(), key.end(),
                                          [](unsigned char ch) { return !std::isspace(ch); }));
      key.erase(
          std::find_if(key.rbegin(), key.rend(), [](unsigned char ch) { return !std::isspace(ch); })
              .base(),
          key.end());
      value.erase(value.begin(), std::find_if(value.begin(), value.end(),
                                              [](unsigned char ch) { return !std::isspace(ch); }));
      value.erase(std::find_if(value.rbegin(), value.rend(),
                               [](unsigned char ch) { return !std::isspace(ch); })
                      .base(),
                  value.end());
      if (!first)
        json << ",";
      json << "\"" << key << "\":";
      // If numeric, no quotes, else quotes
      if (value.find_first_not_of("0123456789.-") == std::string::npos) {
        json << value;
      } else {
        json << "\"" << value << "\"";
      }
      first = false;
    }
  }
  json << "}";
  return json.str();
}

void poll_kernel_status() {
  while (g_running) {
    std::string status = query_kernel("STATUS");
    {
      std::lock_guard<std::mutex> lock(g_status_mutex);
      g_kernel_status = status;
    }
    std::this_thread::sleep_for(std::chrono::seconds{1});
  }
}

} // namespace

int main(int argc, char* argv[]) {
  (void)argc;
  (void)argv;
  std::signal(SIGINT, signal_handler);
  std::signal(SIGTERM, signal_handler);

  std::thread poller(poll_kernel_status);

  heidi::HttpServer server("127.0.0.1", 7778);

  server.register_handler("/api/status", [](const heidi::HttpRequest& req) {
    (void)req;
    heidi::HttpResponse resp;
    std::string status;
    {
      std::lock_guard<std::mutex> lock(g_status_mutex);
      status = g_kernel_status;
    }
    resp.status_code = 200;
    if (status.find("error") != std::string::npos) {
      resp.status_code = 503;
    }
    resp.body = status;
    return resp;
  });

  server.register_handler("/", [](const heidi::HttpRequest& req) {
    (void)req;
    heidi::HttpResponse resp;
    resp.body = "<html><body><h1>Heidi Kernel Dashboard</h1><p>API endpoints: /api/status, "
                "/api/governor/policy, /api/governor/diagnostics</p></body></html>";
    resp.headers["Content-Type"] = "text/html";
    return resp;
  });

  server.register_handler(
      "/api/governor/policy", [](const heidi::HttpRequest& req) -> heidi::HttpResponse {
        heidi::HttpResponse resp;
        if (req.method == "GET") {
          std::string raw = query_kernel("governor/policy");
          if (raw.find("error") == 0) {
            resp.status_code = 503;
            resp.body = raw;
            return resp;
          }
          resp.body = parse_kv_to_json(raw);
          resp.headers["Content-Type"] = "application/json";
        } else if (req.method == "PUT") {
          std::string raw = query_kernel("governor/policy_update", std::string(req.body));
          if (raw.find("error") == 0) {
            resp.status_code = 503;
            resp.body = raw;
            return resp;
          }
          if (raw.find("validation_failed") != std::string::npos) {
            resp.status_code = 400;
            resp.body = parse_kv_to_json(raw);
            resp.headers["Content-Type"] = "application/json";
            return resp;
          }
          if (raw.find("policy_updated") != std::string::npos) {
            resp.status_code = 200;
            resp.body = parse_kv_to_json(raw);
            resp.headers["Content-Type"] = "application/json";
            return resp;
          }
          resp.status_code = 500;
          resp.body = R"({"error": "unknown_response"})";
          resp.headers["Content-Type"] = "application/json";
        } else {
          resp.status_code = 405;
          resp.body = R"({"error": "Method not allowed"})";
          resp.headers["Content-Type"] = "application/json";
        }
        return resp;
      });

  server.register_handler("/api/governor/diagnostics",
                          [](const heidi::HttpRequest& req) -> heidi::HttpResponse {
                            heidi::HttpResponse resp;
                            if (req.method != "GET") {
                              resp.status_code = 405;
                              resp.body = R"({"error": "Method not allowed"})";
                              resp.headers["Content-Type"] = "application/json";
                              return resp;
                            }
                            std::string raw = query_kernel("governor/diagnostics");
                            if (raw.find("error") == 0) {
                              resp.status_code = 503;
                              resp.body = raw;
                              return resp;
                            }
                            resp.body = parse_kv_to_json(raw);
                            resp.headers["Content-Type"] = "application/json";
                            return resp;
                          });

  printf("dashd listening on http://127.0.0.1:7778\n");
  fflush(stdout);

  server.serve_forever(&g_running);

  poller.join();
  return 0;
}
