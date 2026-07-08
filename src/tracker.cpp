#include "../include/tracker.h"
#include "../include/common.h"
#include <iostream>
#include <unistd.h>
#include <cstdio>

void run_tracker(int pipe_read_fd) {
    char buf[256];
    std::cout << "[TRACKER] Waiting for updates...\n";

    while (true) {
        int n = read(pipe_read_fd, buf, sizeof(buf) - 1);
        if (n <= 0) break;
        buf[n] = '\0';
        printf("[TRACKER] Received: %s\n", buf);

        if (std::string(buf) == "ALLOCATION_COMPLETE") {
            std::cout << "[TRACKER] Allocation complete."
                      << " Notifying reporter...\n";
            break;
        }
    }
    close(pipe_read_fd);
    std::cout << "[TRACKER] Shutting down.\n";
}