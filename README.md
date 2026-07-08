# CEHAS — Campus Exam Hall Allocation System

> A multi-process, multithreaded C++ system that allocates students to
> exam halls using parallel processing, inter-process communication,
> and a live TCP query server.

---

## Architecture

```
┌─────────────────────────────────────────────────────┐
│                  CEHAS — Main Process                │
│              fork() → 4 child processes              │
└──────────┬──────────────┬──────────────┬─────────────┘
           │              │              │
           ▼              ▼              ▼
    Process 1        Process 2      Process 3
    Allocator        Tracker        Reporter
    (pthreads +      (reads pipe    (reads shared
     mutex)           from P1)       memory →
                                     report.txt)
           │
           ▼
    Process 4
    TCP Server
    (port 8080,
     backlog 1024,
     per-client threads)
```

---

## Concepts Demonstrated

```
OS / Systems Programming:
├── fork()              → spawn 4 independent child processes
├── pipes               → Allocator → Tracker communication
├── pthreads            → 4 threads for parallel allocation
├── mutex               → race-condition-free shared state
├── shared memory       → Allocator writes, Reporter reads
├── waitpid()           → parent synchronizes all children
└── IPC                 → pipes + shared memory combined

Networking:
└── TCP socket server   → concurrent client queries
    listen(backlog 1024)→ handle multiple connections
    per-client threads  → detached pthread per connection
```

---

## Features

- **Parallel allocation** — 4 pthreads allocate students concurrently with mutex-protected seat assignment
- **Multi-process pipeline** — 4 processes running simultaneously via fork()
- **IPC via pipes** — allocator signals tracker on completion
- **Shared memory** — allocation results shared between processes without copying
- **TCP query server** — clients query student allocation in real time
- **Report generation** — full allocation table written to `output/report.txt`

---

## Project Structure

```
CEHAS/
├── data/
│   ├── students.csv       ← student ID, name, branch, semester
│   └── rooms.csv          ← room ID, block, capacity
├── include/
│   ├── common.h           ← shared structs, shared memory layout
│   ├── allocator.h
│   ├── tracker.h
│   ├── reporter.h
│   └── server.h
├── src/
│   ├── main.cpp           ← fork() all 4 processes
│   ├── allocator.cpp      ← pthreads + mutex + shared memory write
│   ├── tracker.cpp        ← pipe read + status updates
│   ├── reporter.cpp       ← shared memory read + report.txt
│   └── server.cpp         ← TCP socket server + per-client threads
├── output/
│   └── report.txt         ← generated after allocation
└── Makefile
```

---

## Setup and Run

### Prerequisites
```
g++ with C++17 support
POSIX-compliant OS (Linux / macOS)
```

### Build
```bash
make
```

### Run
```bash
./cehas
```

### Query the TCP Server (second terminal)
```bash
# Query a specific student
echo "QUERY 202401001" | nc localhost 8080

# Check allocation status
echo "STATUS" | nc localhost 8080
```

### Sample Output
```
[LOADER] Loaded 20 students.
[LOADER] Loaded 4 rooms.
[ALLOCATOR] Hari Sharma → Room R001 Seat 1
[ALLOCATOR] Arjun Patel → Room R001 Seat 2
[TRACKER] Waiting for updates...
[ALLOCATOR] Done. Allocated 20 students.
[TRACKER] Received: ALLOCATION_COMPLETE
[REPORTER] Generating report...
[SERVER] Listening on port 8080.
```

### TCP Query Response
```
$ echo "QUERY 202401001" | nc localhost 8080
Student : Hari Sharma
Room    : R001 (Block A)
Seat    : 1

$ echo "STATUS" | nc localhost 8080
Status    : COMPLETE
Allocated : 20 / 20
```

---

## Data Format

### students.csv
```
student_id,name,branch,semester
202401001,Hari Sharma,ICT,4
```

### rooms.csv
```
room_id,block,capacity
R001,A,40
```

---

## IPC Mechanisms Used

| Mechanism | Used Between | Purpose |
|---|---|---|
| `pipe()` | Allocator → Tracker | Signal on completion |
| Shared Memory (`shmget`) | Allocator → Reporter | Pass allocation results |
| TCP Socket | Server → Clients | Live allocation queries |
| `pthread_mutex_t` | Between threads in Allocator | Protect shared seat table |

---

*Built as part of systems programming exploration — OS + IPC + networking in C++.*