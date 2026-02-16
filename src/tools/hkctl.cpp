#include "common/status.hpp"
#include <iostream>
#include <string>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <chrono>
#include <fcntl.h>
#include <sys/time.h>
#include <cstring>

class HKClient {
private:
    std::string sock_path_;
    int timeout_ms_ = 2000;
    
public:
    HKClient(const std::string& sock_path) : sock_path_(sock_path) {}
    
    bool send_command(const std::string& cmd, std::string& response) {
        int client_fd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (client_fd < 0) {
            std::cerr << "Failed to create socket\n";
            return false;
        }
        
        // Set timeout
        struct timeval tv;
        tv.tv_sec = timeout_ms_ / 1000;
        tv.tv_usec = (timeout_ms_ % 1000) * 1000;
        setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        setsockopt(client_fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
        
        struct sockaddr_un addr;
        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, sock_path_.c_str(), sizeof(addr.sun_path) - 1);
        
        if (connect(client_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            std::cerr << "Failed to connect to daemon at " << sock_path_ << "\n";
            close(client_fd);
            return false;
        }
        
        // Send command
        std::string request = cmd + "\n";
        if (send(client_fd, request.c_str(), request.length(), 0) < 0) {
            std::cerr << "Failed to send command\n";
            close(client_fd);
            return false;
        }
        
        // Read response
        char buffer[4096];
        ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            response = std::string(buffer);
            response.erase(response.find_last_not_of(" \t\n\r") + 1); // Trim
        } else {
            std::cerr << "Failed to read response\n";
            close(client_fd);
            return false;
        }
        
        close(client_fd);
        return true;
    }
};

int main(int argc, char* argv[]) {
    std::string sock_path = "/tmp/heidi-kernel.sock";
    std::string command;
    
    // Parse command line args
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--sock" && i + 1 < argc) {
            sock_path = argv[++i];
        } else if (arg == "--help") {
            std::cout << "Usage: " << argv[0] << " [--sock <path>] <command>\n";
            std::cout << "Commands:\n";
            std::cout << "  status  Get daemon status\n";
            return 0;
        } else if (command.empty()) {
            command = arg;
        }
    }
    
    if (command.empty()) {
        command = "STATUS";
    } else if (command == "status") {
        command = "STATUS";
    }
    
    // Check environment variable
    const char* env_sock = getenv("HEIDI_KERNEL_SOCK");
    if (env_sock) {
        sock_path = env_sock;
    }
    
    HKClient client(sock_path);
    std::string response;
    
    if (client.send_command(command, response)) {
        std::cout << response << "\n";
        return 0;
    } else {
        return 1;
    }
}
