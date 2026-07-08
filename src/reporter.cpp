#include "../include/reporter.h"
#include "../include/common.h"
#include <iostream>
#include <fstream>
#include <sys/shm.h>
#include <unistd.h>
#include <cstdio>

void run_reporter() {
    std::cout << "[REPORTER] Waiting for allocation to complete...\n";

    // Attach shared memory
    int shmid = shmget(SHM_KEY, sizeof(SharedData), 0666);
    while (shmid == -1) {
        sleep(1);
        shmid = shmget(SHM_KEY, sizeof(SharedData), 0666);
    }

    SharedData* shm = (SharedData*)shmat(shmid, nullptr, 0);

    // Wait until allocation is complete
    while (shm->status != COMPLETE) {
        sleep(1);
    }

    std::cout << "[REPORTER] Generating report...\n";

    // Write report to file
    std::ofstream report("output/report.txt");
    report << "========================================\n";
    report << "   CEHAS — Exam Hall Allocation Report  \n";
    report << "========================================\n\n";
    report << "Total Students : " << shm->total_students  << "\n";
    report << "Total Allocated: " << shm->total_allocated << "\n\n";
    report << "----------------------------------------\n";
    report << "Student Allocations:\n";
    report << "----------------------------------------\n";

    printf("%-12s %-25s %-8s %-8s\n",
           "Student ID", "Name", "Room", "Seat");
    std::cout << std::string(55, '-') << "\n";

    for (int i = 0; i < shm->total_allocated; i++) {
        Allocation& a = shm->allocs[i];

        // Print to terminal
        printf("%-12d %-25s %-8s %-8d\n",
               a.student_id,
               a.student_name.c_str(),
               a.room_id.c_str(),
               a.seat_no);

        // Write to file
        report << a.student_id    << ","
               << a.student_name  << ","
               << a.room_id       << ","
               << a.block         << ","
               << a.seat_no       << "\n";
    }

    report << "\n========================================\n";
    report << "Report generated successfully.\n";
    report.close();

    std::cout << "\n[REPORTER] Report saved to output/report.txt\n";
    shmdt(shm);

    // Cleanup shared memory
    shmctl(shmid, IPC_RMID, nullptr);
    std::cout << "[REPORTER] Shared memory cleaned up.\n";
}