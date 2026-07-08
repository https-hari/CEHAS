#pragma once
#include "common.h"
#include <vector>

// Thread argument structure
struct BatchArgs {
    int                   start;
    int                   end;
    std::vector<Student>* students;
    std::vector<Room>*    rooms;
    int                   pipe_write_fd;
};

// Main allocator function (called from child process)
void run_allocator(std::vector<Student>& students,
                   std::vector<Room>&    rooms,
                   int                   pipe_write_fd);