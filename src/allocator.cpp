#include "../include/common.h"
#include "../include/allocator.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <pthread.h>
#include <unistd.h>
#include <sys/shm.h>
#include <cstring>
#include <cstdio>

// ── CSV Loaders ───────────────────────────────────────────
std::vector<Student> load_students(const std::string& path) {
    std::vector<Student> students;
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "[ERROR] Cannot open " << path << "\n";
        return students;
    }
    std::string line;
    std::getline(file, line);
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string token;
        Student s;
        std::getline(ss, token, ','); s.id       = std::stoi(token);
        std::getline(ss, token, ','); s.name     = token;
        std::getline(ss, token, ','); s.branch   = token;
        std::getline(ss, token, ',');
        token.erase(token.find_last_not_of(" \n\r\t") + 1);
        s.semester = std::stoi(token);
        students.push_back(s);
    }
    std::cout << "[LOADER] Loaded " << students.size() << " students.\n";
    return students;
}

std::vector<Room> load_rooms(const std::string& path) {
    std::vector<Room> rooms;
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "[ERROR] Cannot open " << path << "\n";
        return rooms;
    }
    std::string line;
    std::getline(file, line);
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string token;
        Room r;
        std::getline(ss, token, ','); r.id    = token;
        std::getline(ss, token, ','); r.block  = token;
        std::getline(ss, token, ',');
        token.erase(token.find_last_not_of(" \n\r\t") + 1);
        r.capacity = std::stoi(token);
        rooms.push_back(r);
    }
    std::cout << "[LOADER] Loaded " << rooms.size() << " rooms.\n";
    return rooms;
}

// ── Shared allocation state ───────────────────────────────
static pthread_mutex_t alloc_mutex    = PTHREAD_MUTEX_INITIALIZER;
static SharedData*     shm            = nullptr;
static int             seat_counters[MAX_ALLOCS] = {0};
static int             room_index     = 0;

// ── Thread worker ─────────────────────────────────────────
void* allocate_batch(void* arg) {
    BatchArgs* b = (BatchArgs*)arg;
    for (int i = b->start; i < b->end; i++) {
        Student& s = (*b->students)[i];
        pthread_mutex_lock(&alloc_mutex);
        while (room_index < (int)b->rooms->size() &&
               seat_counters[room_index] >=
               (*b->rooms)[room_index].capacity) {
            room_index++;
        }
        if (room_index >= (int)b->rooms->size()) {
            std::cerr << "[ALLOCATOR] No seats left!\n";
            pthread_mutex_unlock(&alloc_mutex);
            continue;
        }
        Room& r  = (*b->rooms)[room_index];
        int   seat = ++seat_counters[room_index];
        Allocation a;
        a.student_id   = s.id;
        a.student_name = s.name;
        a.room_id      = r.id;
        a.block        = r.block;
        a.seat_no      = seat;
        int idx = shm->total_allocated++;
        shm->allocs[idx] = a;
        printf("[ALLOCATOR] %s → Room %s Seat %d\n",
               s.name.c_str(), r.id.c_str(), seat);
        pthread_mutex_unlock(&alloc_mutex);
    }
    return nullptr;
}

// ── Main allocator ────────────────────────────────────────
void run_allocator(std::vector<Student>& students,
                   std::vector<Room>&    rooms,
                   int                   pipe_write_fd) {
    int shmid = shmget(SHM_KEY, sizeof(SharedData),
                       0666 | IPC_CREAT);
    shm = (SharedData*)shmat(shmid, nullptr, 0);
    memset(shm, 0, sizeof(SharedData));
    shm->status         = IN_PROGRESS;
    shm->total_students = students.size();
    shm->total_allocated = 0;

    const int NUM_THREADS = 4;
    pthread_t threads[NUM_THREADS];
    BatchArgs args[NUM_THREADS];
    int batch = students.size() / NUM_THREADS;

    for (int i = 0; i < NUM_THREADS; i++) {
        args[i].start    = i * batch;
        args[i].end      = (i == NUM_THREADS - 1)
                           ? (int)students.size()
                           : (i + 1) * batch;
        args[i].students      = &students;
        args[i].rooms         = &rooms;
        args[i].pipe_write_fd = pipe_write_fd;
        pthread_create(&threads[i], nullptr,
                       allocate_batch, &args[i]);
    }

    for (int i = 0; i < NUM_THREADS; i++)
        pthread_join(threads[i], nullptr);

    shm->status = COMPLETE;
    const char* msg = "ALLOCATION_COMPLETE";
    write(pipe_write_fd, msg, strlen(msg));
    close(pipe_write_fd);

    printf("[ALLOCATOR] Done. Allocated %d students.\n",
           shm->total_allocated);
    shmdt(shm);
}