#pragma once
#include <string>
#include <vector>

// ── Data structures ──────────────────────────────────────
struct Student {
    int         id;
    std::string name;
    std::string branch;
    int         semester;
};

struct Room {
    std::string id;
    std::string block;
    int         capacity;
};

struct Allocation {
    int         student_id;
    std::string student_name;
    std::string room_id;
    std::string block;
    int         seat_no;
};

// ── Shared memory structure ───────────────────────────────
#define SHM_KEY     0x1234
#define MAX_ALLOCS  2000

enum Status { PENDING, IN_PROGRESS, COMPLETE };

struct SharedData {
    Status     status;
    int        total_students;
    int        total_allocated;
    Allocation allocs[MAX_ALLOCS];
};

// ── CSV loaders ───────────────────────────────────────────
std::vector<Student> load_students(const std::string& path);
std::vector<Room>    load_rooms(const std::string& path);