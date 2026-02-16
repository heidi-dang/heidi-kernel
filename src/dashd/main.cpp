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

#include "heidi-kernel/http.h"

namespace {

volatile std::sig_atomic_t g_running = 1;
std::mutex g_status_mutex;
std::string g_kernel_status = "{\"error\":\"not connected\"}";

void signal_handler(int) {
    g_running = 0;
}

std::string query_kernel_status() {
    const char* xdg_runtime = getenv("XDG_RUNTIME_DIR");
    std::string socket_path = xdg_runtime && xdg_runtime[0] ?
        std::string(xdg_runtime) + "/heidi-kernel.sock" :
        "/tmp/heidi-kernel.sock";

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

    const char* request = "STATUS\n";
    ssize_t w = write(fd, request, strlen(request));
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

void poll_kernel_status() {
    while (g_running) {
        std::string status = query_kernel_status();
        {
            std::lock_guard<std::mutex> lock(g_status_mutex);
            g_kernel_status = status;
        }
        std::this_thread::sleep_for(std::chrono::seconds{1});
    }
}

} // namespace

int main(int argc, char* argv[]) {
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    std::thread poller(poll_kernel_status);

    heidi::HttpServer server("127.0.0.1", 7778);

    server.register_handler("/api/status", [](const heidi::HttpRequest& req) {
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
        heidi::HttpResponse resp;
        resp.status_code = 200;
        resp.headers["Content-Type"] = "text/html";
        resp.body = R"(<!DOCTYPE html>
<html>
<head>
<title>Heidi Kernel Dashboard</title>
<style>
body { font-family: monospace; padding: 20px; }
#status { background: #f0f0f0; padding: 10px; border-radius: 4px; }
</style>
</head>
<body>
<h1>Kernel Status</h1>
<pre id="status">Loading...</pre>
<script>
async function update() {
  try {
    const r = await fetch('/api/status');
    const j = await r.json();
    document.getElementById('status').textContent = JSON.stringify(j, null, 2);
  } catch(e) {
    document.getElementById('status').textContent = 'Error: ' + e;
  }
}
update();
setInterval(update, 1000);
</script>
</body>
</html>)";
        return resp;
    });

    printf("dashd listening on http://127.0.0.1:7778\n");
    fflush(stdout);

    server.serve_forever();

    poller.join();
    return 0;
}
