#include "common/status.hpp"
#include <iostream>
#include <string>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <signal.h>
#include <chrono>
#include <thread>
#include <fcntl.h>

class KernelDaemon {
private:
    std::string sock_path_;
    int server_fd_ = -1;
    volatile bool running_ = true;
    std::chrono::steady_clock::time_point start_time_;
    
public:
    KernelDaemon(const std::string& sock_path) : sock_path_(sock_path) {
        start_time_ = std::chrono::steady_clock::now();
    }
    
    ~KernelDaemon() {
        cleanup();
    }
    
    bool start() {
        // Setup signal handlers
        signal(SIGINT, [](int) { exit(0); });
        signal(SIGTERM, [](int) { exit(0); });
        
        // Create Unix domain socket
        server_fd_ = socket(AF_UNIX, SOCK_STREAM, 0);
        if (server_fd_ < 0) {
            std::cerr << "Failed to create socket\n";
            return false;
        }
        
        // Set socket to non-blocking for accept
        int flags = fcntl(server_fd_, F_GETFL, 0);
        fcntl(server_fd_, F_SETFL, flags | O_NONBLOCK);
        
        // Bind to socket path
        struct sockaddr_un addr;
        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, sock_path_.c_str(), sizeof(addr.sun_path) - 1);
        
        // Unlink existing socket if present
        unlink(sock_path_.c_str());
        
        if (bind(server_fd_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            std::cerr << "Failed to bind socket\n";
            close(server_fd_);
            return false;
        }
        
        // Listen for connections
        if (listen(server_fd_, 5) < 0) {
            std::cerr << "Failed to listen\n";
            close(server_fd_);
            return false;
        }
        
        std::cout << "Heidi Kernel Daemon started\n";
        std::cout << "Socket: " << sock_path_ << "\n";
        std::cout << "PID: " << getpid() << "\n";
        
        return true;
    }
    
    void run() {
        while (running_) {
            // Accept connections with timeout
            struct timeval tv;
            tv.tv_sec = 1;
            tv.tv_usec = 0;
            
            fd_set read_fds;
            FD_ZERO(&read_fds);
            FD_SET(server_fd_, &read_fds);
            
            int result = select(server_fd_ + 1, &read_fds, nullptr, nullptr, &tv);
            if (result < 0) {
                if (errno == EINTR) continue;
                break;
            }
            
            if (result > 0 && FD_ISSET(server_fd_, &read_fds)) {
                handle_connection();
            }
            
            // Near-zero idle CPU - only wake up for connections or signals
        }
    }
    
private:
    void handle_connection() {
        struct sockaddr_un client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        int client_fd = accept(server_fd_, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                std::cerr << "Accept failed\n";
            }
            return;
        }
        
        // Set read timeout
        struct timeval tv;
        tv.tv_sec = 2; // 2 second timeout
        tv.tv_usec = 0;
        setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        
        char buffer[4096];
        ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            std::string request(buffer);
            
            // Remove whitespace
            request.erase(0, request.find_first_not_of(" \t\n\r"));
            request.erase(request.find_last_not_of(" \t\n\r") + 1);
            
            std::string response;
            if (request == "STATUS") {
                response = get_status() + "\n";
            } else {
                response = "{\"ok\":false,\"error\":\"unknown_command\"}\n";
            }
            
            send(client_fd, response.c_str(), response.length(), 0);
        }
        
        close(client_fd);
    }
    
    std::string get_status() {
        heidi::Status status;
        status.ok = true;
        status.pid = getpid();
        
        auto now = std::chrono::steady_clock::now();
        auto uptime = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time_);
        status.uptime_ms = uptime.count();
        
        status.version = "1.0.0";
        status.sock_path = sock_path_;
        status.build_git = "unknown"; // Would be set at build time
        
        return status.to_json();
    }
    
    void cleanup() {
        running_ = false;
        if (server_fd_ >= 0) {
            close(server_fd_);
        }
        unlink(sock_path_.c_str());
    }
};

int main(int argc, char* argv[]) {
    std::string sock_path = "/tmp/heidi-kernel.sock";
    
    // Parse command line args
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--sock" && i + 1 < argc) {
            sock_path = argv[++i];
        } else if (arg == "--help") {
            std::cout << "Usage: " << argv[0] << " [--sock <path>]\n";
            return 0;
        }
    }
    
    // Check environment variable
    const char* env_sock = getenv("HEIDI_KERNEL_SOCK");
    if (env_sock) {
        sock_path = env_sock;
    }
    
    KernelDaemon daemon(sock_path);
    if (!daemon.start()) {
        return 1;
    }
    
    daemon.run();
    return 0;
}
