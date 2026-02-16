#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <csignal>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <thread>

#include "heidi-kernel/http.h"

namespace {

std::sig_atomic_t g_running = 1;
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
        return "{\"error\":\"socket failed\"}";
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path.c_str(), sizeof(addr.sun_path) - 1);

    if (connect(fd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) {
        close(fd);
        return "{\"error\":\"kernel not running\"}";
    }

    const char* request = "STATUS\n";
    write(fd, request, strlen(request));

    char buf[4096];
    ssize_t n = read(fd, buf, sizeof(buf) - 1);
    close(fd);

    if (n <= 0) {
        return "{\"error\":\"no response\"}";
    }
    buf[n] = '\0';
    return std::string(buf, n);
}

void poll_kernel_status() {
    while (g_running) {
        g_kernel_status = query_kernel_status();
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
        resp.status_code = 200;
        resp.body = g_kernel_status;
        return resp;
    });

    server.register_handler("/", [](const heidi::HttpRequest& req) {
        heidi::HttpResponse resp;
        resp.status_code = 200;
        resp.headers["Content-Type"] = "text/html";
        resp.body = R"(<!DOCTYPE html>
<html>
<head><title>Heidi Kernel Dashboard</title></head>
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
