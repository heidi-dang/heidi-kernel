#include "heidi-kernel/daemon.h"

#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    std::string socket_path = "/tmp/heidi-kernel.sock";
    
    if (argc > 1) {
        socket_path = argv[1];
    }
    
    try {
        heidi::Daemon daemon(socket_path);
        daemon.run();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}