#include "../include/server.h"
#include "../include/common.h"
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <cstdio>
#include <pthread.h>
#include <sys/shm.h>
#include <string>
#include <sstream>

// ── Shared memory pointer (read-only) ────────────────────
static SharedData* shm = nullptr;

// ── Lookup allocation by student ID ──────────────────────
static std::string lookup(int student_id) {
    for (int i = 0; i < shm->total_allocated; i++) {
        if (shm->allocs[i].student_id == student_id) {
            Allocation& a = shm->allocs[i];
            char buf[256];
            snprintf(buf, sizeof(buf),
                "Student : %s\n"
                "Room    : %s (Block %s)\n"
                "Seat    : %d\n",
                a.student_name.c_str(),
                a.room_id.c_str(),
                a.block.c_str(),
                a.seat_no);
            return std::string(buf);
        }
    }
    return "ERROR: Student not found.\n";
}

// ── Handle one client connection ──────────────────────────
void* handle_client(void* arg) {
    int client_fd = (intptr_t)arg;
    char buf[256] = {0};

    recv(client_fd, buf, sizeof(buf) - 1, 0);
    std::string req(buf);

    // Trim whitespace
    req.erase(req.find_last_not_of(" \n\r\t") + 1);

    printf("[SERVER] Query: %s\n", req.c_str());

    std::string response;

    if (req.substr(0, 5) == "QUERY") {
        try {
            int id = std::stoi(req.substr(6));
            response = lookup(id);
        } catch (...) {
            response = "ERROR: Invalid query format. "
                       "Use: QUERY <student_id>\n";
        }
    } else if (req == "STATUS") {
        char buf2[128];
        snprintf(buf2, sizeof(buf2),
            "Status    : %s\n"
            "Allocated : %d / %d\n",
            shm->status == COMPLETE ? "COMPLETE" : "IN_PROGRESS",
            shm->total_allocated,
            shm->total_students);
        response = std::string(buf2);
    } else {
        response = "ERROR: Unknown command. "
                   "Commands: QUERY <id> | STATUS\n";
    }

    send(client_fd, response.c_str(), response.size(), 0);
    close(client_fd);
    return nullptr;
}

// ── Main server loop ──────────────────────────────────────
void run_server() {
    // Wait for shared memory to be ready
    int shmid = -1;
    while (shmid == -1) {
        shmid = shmget(SHM_KEY, sizeof(SharedData), 0666);
        if (shmid == -1) sleep(1);
    }
    shm = (SharedData*)shmat(shmid, nullptr, 0);

    // Wait for allocation to complete
    while (shm->status != COMPLETE) sleep(1);

    std::cout << "[SERVER] Allocation ready. "
              << "Starting TCP server on port 8080...\n";

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) { perror("socket"); return; }

    // Allow port reuse
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR,
               &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(8080);

    if (bind(server_fd, (sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("bind"); return;
    }

    listen(server_fd, 1024);
    std::cout << "[SERVER] Listening on port 8080. "
              << "Send: QUERY <student_id> or STATUS\n";

    while (true) {
        int client = accept(server_fd, nullptr, nullptr);
        if (client == -1) continue;

        pthread_t t;
        pthread_create(&t, nullptr, handle_client,
                       (void*)(intptr_t)client);
        pthread_detach(t);
    }

    shmdt(shm);
}