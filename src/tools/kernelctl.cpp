#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <string>

std::string send_request(const std::string& socket_path, const std::string& request) {
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0) {
        throw std::runtime_error("Failed to create socket");
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path.c_str(), sizeof(addr.sun_path) - 1);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(sock);
        throw std::runtime_error("Failed to connect to daemon");
    }

    write(sock, request.c_str(), request.size());
    
    char buffer[1024];
    ssize_t n = read(sock, buffer, sizeof(buffer) - 1);
    close(sock);
    
    if (n <= 0) {
        throw std::runtime_error("Failed to read response");
    }
    
    buffer[n] = '\0';
    return std::string(buffer);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: heidi-kernelctl <command> [--socket <path>]" << std::endl;
        std::cout << "Commands: ping, status" << std::endl;
        return 1;
    }

    std::string command = argv[1];
    std::string socket_path = "/tmp/heidi-kernel.sock";
    
    for (int i = 2; i < argc; ++i) {
        if (std::string(argv[i]) == "--socket" && i + 1 < argc) {
            socket_path = argv[i + 1];
            ++i;
        }
    }

    try {
        if (command == "ping") {
            std::string response = send_request(socket_path, "ping\n");
            std::cout << response;
        } else if (command == "status") {
            std::string response = send_request(socket_path, "status\n");
            std::cout << response;
        } else if (command == "metrics") {
            if (argc > 2 && std::string(argv[2]) == "latest") {
                std::string response = send_request(socket_path, "metrics latest\n");
                std::cout << response;
            } else if (argc > 3 && std::string(argv[2]) == "tail") {
                std::string n_str = argv[3];
                std::string response = send_request(socket_path, "metrics tail " + n_str + "\n");
                std::cout << response;
            } else {
                std::cout << "Usage: heidi-kernelctl metrics latest|tail <n> [--socket <path>]" << std::endl;
                return 1;
            }
        } else {
            std::cout << "Unknown command: " << command << std::endl;
            std::cout << "Available: ping, status, metrics latest, metrics tail <n>" << std::endl;
            return 1;
        }
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}