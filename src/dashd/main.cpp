#include "common/status.hpp"
#include <iostream>
#include <string>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <signal.h>
#include <thread>
#include <fcntl.h>
#include <sstream>
#include <fstream>
#include <sys/time.h>
#include <cstring>
#include <arpa/inet.h>
#include <netinet/in.h>

class HTTPServer {
private:
    std::string listen_addr_;
    std::string sock_path_;
    std::string static_dir_;
    int server_fd_ = -1;
    volatile bool running_ = true;
    int timeout_ms_ = 2000;
    
public:
    HTTPServer(const std::string& listen_addr, const std::string& sock_path, const std::string& static_dir)
        : listen_addr_(listen_addr), sock_path_(sock_path), static_dir_(static_dir) {}
    
    ~HTTPServer() {
        cleanup();
    }
    
    bool start() {
        signal(SIGINT, [](int) { exit(0); });
        signal(SIGTERM, [](int) { exit(0); });
        
        // Parse listen address
        std::string host = "127.0.0.1";
        int port = 8788;
        
        size_t colon_pos = listen_addr_.find(':');
        if (colon_pos != std::string::npos) {
            host = listen_addr_.substr(0, colon_pos);
            port = std::stoi(listen_addr_.substr(colon_pos + 1));
        }
        
        // Create TCP socket
        server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd_ < 0) {
            std::cerr << "Failed to create socket\n";
            return false;
        }
        
        // Set reuse address
        int opt = 1;
        setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        
        // Bind to address
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = inet_addr(host.c_str());
        addr.sin_port = htons(port);
        
        if (bind(server_fd_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            std::cerr << "Failed to bind to " << listen_addr_ << "\n";
            close(server_fd_);
            return false;
        }
        
        // Listen
        if (listen(server_fd_, 5) < 0) {
            std::cerr << "Failed to listen\n";
            close(server_fd_);
            return false;
        }
        
        std::cout << "dashd started\n";
        std::cout << "HTTP: " << listen_addr_ << "\n";
        std::cout << "UDS: " << sock_path_ << "\n";
        if (!static_dir_.empty()) {
            std::cout << "Static: " << static_dir_ << "\n";
        }
        
        return true;
    }
    
    void run() {
        while (running_) {
            // Accept with timeout
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
        }
    }
    
private:
    void handle_connection() {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        int client_fd = accept(server_fd_, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            return;
        }
        
        // Read HTTP request
        char buffer[4096];
        ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            std::string request(buffer);
            
            // Parse first line
            std::istringstream iss(request);
            std::string method, path, version;
            iss >> method >> path >> version;
            
            std::string response;
            if (method == "GET") {
                if (path == "/api/status") {
                    response = handle_status_api();
                } else if (!static_dir_.empty()) {
                    response = handle_static_file(path);
                } else {
                    response = "HTTP/1.1 404 Not Found\r\n\r\n404 - Static files not enabled";
                }
            } else {
                response = "HTTP/1.1 405 Method Not Allowed\r\n\r\n405 - Method not allowed";
            }
            
            send(client_fd, response.c_str(), response.length(), 0);
        }
        
        close(client_fd);
    }
    
    std::string handle_status_api() {
        std::string json_response;
        
        if (get_kernel_status(json_response)) {
            std::ostringstream oss;
            oss << "HTTP/1.1 200 OK\r\n";
            oss << "Content-Type: application/json\r\n";
            oss << "Access-Control-Allow-Origin: *\r\n";
            oss << "Content-Length: " << json_response.length() << "\r\n";
            oss << "\r\n";
            oss << json_response;
            return oss.str();
        } else {
            return "HTTP/1.1 502 Bad Gateway\r\n\r\n502 - Kernel daemon unavailable";
        }
    }
    
    bool get_kernel_status(std::string& response) {
        int client_fd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (client_fd < 0) {
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
            close(client_fd);
            return false;
        }
        
        // Send STATUS command
        const char* cmd = "STATUS\n";
        if (send(client_fd, cmd, strlen(cmd), 0) < 0) {
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
            close(client_fd);
            return true;
        }
        
        close(client_fd);
        return false;
    }
    
    std::string handle_static_file(const std::string& path) {
        if (path == "/") {
            return serve_file("index.html");
        }
        
        // Prevent directory traversal
        if (path.find("..") != std::string::npos) {
            return "HTTP/1.1 403 Forbidden\r\n\r\n403 - Directory traversal not allowed";
        }
        
        return serve_file(path.substr(1)); // Remove leading /
    }
    
    std::string serve_file(const std::string& filename) {
        std::string full_path = static_dir_ + "/" + filename;
        
        std::ifstream file(full_path, std::ios::binary);
        if (!file.is_open()) {
            return "HTTP/1.1 404 Not Found\r\n\r\n404 - File not found";
        }
        
        // Read file content
        std::string content((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
        
        // Determine content type
        std::string content_type = "text/plain";
        if (filename.ends_with(".html")) content_type = "text/html";
        else if (filename.ends_with(".css")) content_type = "text/css";
        else if (filename.ends_with(".js")) content_type = "application/javascript";
        else if (filename.ends_with(".json")) content_type = "application/json";
        
        std::ostringstream oss;
        oss << "HTTP/1.1 200 OK\r\n";
        oss << "Content-Type: " << content_type << "\r\n";
        oss << "Content-Length: " << content.length() << "\r\n";
        oss << "\r\n";
        oss << content;
        
        return oss.str();
    }
    
    void cleanup() {
        running_ = false;
        if (server_fd_ >= 0) {
            close(server_fd_);
        }
    }
};

int main(int argc, char* argv[]) {
    std::string listen_addr = "127.0.0.1:8788";
    std::string sock_path = "/tmp/heidi-kernel.sock";
    std::string static_dir;
    
    // Parse command line args
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--listen" && i + 1 < argc) {
            listen_addr = argv[++i];
        } else if (arg == "--sock" && i + 1 < argc) {
            sock_path = argv[++i];
        } else if (arg == "--static" && i + 1 < argc) {
            static_dir = argv[++i];
        } else if (arg == "--help") {
            std::cout << "Usage: " << argv[0] << " [--listen host:port] [--sock path] [--static dir]\n";
            return 0;
        }
    }
    
    HTTPServer server(listen_addr, sock_path, static_dir);
    if (!server.start()) {
        return 1;
    }
    
    server.run();
    return 0;
}
