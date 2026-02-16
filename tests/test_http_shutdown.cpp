#include "heidi-kernel/http.h"
#include <gtest/gtest.h>
#include <thread>
#include <atomic>
#include <chrono>
#include <csignal>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

namespace heidi {

TEST(HttpServerShutdown, StopServerGracefully) {
    volatile std::sig_atomic_t running = 1;
    HttpServer server("127.0.0.1", 7779); // Use a different port to avoid conflict

    // Run server in a thread
    std::thread server_thread([&]() {
        server.serve_forever(&running);
    });

    // Give it a moment to start
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Connect to it to verify it's running (optional, but good)
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    ASSERT_GE(sock, 0);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(7779);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Retry connect a few times
    int connected = -1;
    for (int i = 0; i < 5; ++i) {
        connected = connect(sock, (struct sockaddr*)&addr, sizeof(addr));
        if (connected == 0) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    EXPECT_EQ(connected, 0);
    close(sock);

    // Stop the server
    running = 0;

    // Join the thread (this will hang if the fix is not working)
    server_thread.join();

    SUCCEED();
}

}
