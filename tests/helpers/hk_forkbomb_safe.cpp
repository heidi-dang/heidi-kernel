#include <iostream>
#include <vector>
#include <unistd.h>
#include <sys/wait.h>
#include <cstdlib>
#include <cstring>

int main(int argc, char* argv[]) {
    int children = 5;
    int hold_ms = 10000;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--children" && i + 1 < argc) {
            children = std::atoi(argv[++i]);
        } else if (arg == "--hold-ms" && i + 1 < argc) {
            hold_ms = std::atoi(argv[++i]);
        }
    }

    // Set process group
    if (setpgid(0, 0) != 0) {
        std::cerr << "Failed to setpgid\n";
        return 1;
    }

    std::vector<pid_t> child_pids;
    for (int i = 0; i < children; ++i) {
        pid_t pid = fork();
        if (pid == 0) {
            // Child: pause indefinitely
            pause();
            return 0;
        } else if (pid > 0) {
            child_pids.push_back(pid);
        } else {
            std::cerr << "Fork failed\n";
            return 1;
        }
    }

    // Hold for specified ms
    usleep(hold_ms * 1000);

    // Wait for children (though they are paused)
    for (pid_t pid : child_pids) {
        waitpid(pid, nullptr, 0);
    }

    return 0;
}