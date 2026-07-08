#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include "../include/common.h"
#include "../include/allocator.h"
#include "../include/tracker.h"
#include "../include/reporter.h"
#include "../include/server.h"

int main() {
    auto students = load_students("data/students.csv");
    auto rooms    = load_rooms("data/rooms.csv");

    int pipe_fd[2];
    if (pipe(pipe_fd) == -1) { perror("pipe"); return 1; }

    // ── Child 1: Allocator ────────────────────────────
    pid_t p1 = fork();
    if (p1 == 0) {
        close(pipe_fd[0]);
        run_allocator(students, rooms, pipe_fd[1]);
        exit(0);
    }

    // ── Child 2: Tracker ──────────────────────────────
    pid_t p2 = fork();
    if (p2 == 0) {
        close(pipe_fd[1]);
        run_tracker(pipe_fd[0]);
        exit(0);
    }

    // ── Child 3: Reporter ─────────────────────────────
    pid_t p3 = fork();
    if (p3 == 0) {
        close(pipe_fd[0]);
        close(pipe_fd[1]);
        run_reporter();
        exit(0);
    }

    // ── Child 4: TCP Server ───────────────────────────
    pid_t p4 = fork();
    if (p4 == 0) {
        close(pipe_fd[0]);
        close(pipe_fd[1]);
        run_server();
        exit(0);
    }

    // Parent closes pipe
    close(pipe_fd[0]);
    close(pipe_fd[1]);

    // Wait for allocator, tracker, reporter
    waitpid(p1, nullptr, 0);
    std::cout << "[MAIN] Allocator done.\n";
    waitpid(p2, nullptr, 0);
    std::cout << "[MAIN] Tracker done.\n";
    waitpid(p3, nullptr, 0);
    std::cout << "[MAIN] Reporter done.\n";

    std::cout << "[MAIN] Server running. "
              << "Press Ctrl+C to stop.\n";

    // Wait for server (runs until killed)
    waitpid(p4, nullptr, 0);
    return 0;
}