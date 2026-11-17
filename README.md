# 🔍 Live Deadlock Detector

A real-time deadlock detection system for multi-threaded C programs using LD_PRELOAD interception and DFS-based cycle detection.

## 🎯 Features

- ✅ **Runtime Interception**: Captures lock events from ANY C program without source code modification
- ✅ **LD_PRELOAD Wrapper**: Intercepts pthread_mutex_lock/unlock calls transparently
- ✅ **Binary Event Format**: Generates kernel-compatible binary event streams
- ✅ **DFS Cycle Detection**: Detects deadlock cycles in wait-for graphs
- ✅ **Comprehensive Testing**: 148 unit and integration tests
- ✅ **Multiple Scenarios**: Banking deadlock, 3-thread cycles, clean execution
- ✅ **Recommendations**: Provides actionable advice to fix detected deadlocks

## 🏗️ Architecture

```
┌──────────────────────────────────────────────────┐
│  Any C Program (with pthread mutexes)           │
└──────────────────┬───────────────────────────────┘
                   │
                   ↓
┌──────────────────────────────────────────────────┐
│  LD_PRELOAD Wrapper (liblocktrace.so)           │
│  - Intercepts pthread_mutex_lock/unlock          │
│  - Captures: tid, lock_addr, timestamp, priority │
│  - Writes to: lock_events.bin                    │
└──────────────────┬───────────────────────────────┘
                   │
                   ↓
┌──────────────────────────────────────────────────┐
│  Event Collector (transport_file.c)              │
│  - Reads binary event stream                     │
│  - Validates event format                        │
└──────────────────┬───────────────────────────────┘
                   │
                   ↓
┌──────────────────────────────────────────────────┐
│  Lock State Manager (lock_state.c)               │
│  - Tracks lock ownership                         │
│  - Tracks waiting threads                        │
│  - Handles edge cases                            │
└──────────────────┬───────────────────────────────┘
                   │
                   ↓
┌──────────────────────────────────────────────────┐
│  Wait-For Graph (wait_for_graph.c)               │
│  - Builds directed graph (A→B: A waits for B)    │
│  - Updates on each event                         │
└──────────────────┬───────────────────────────────┘
                   │
                   ↓
┌──────────────────────────────────────────────────┐
│  Deadlock Detector (detector_main.c)             │
│  - DFS cycle detection                           │
│  - Reports cycle path                            │
│  - Provides recommendations                      │
└──────────────────────────────────────────────────┘
```

## 🚀 Quick Start

### Build Everything
```bash
cd ~/live-deadlock-detector
make clean
make

# Build wrapper
gcc -shared -fPIC wrapper/locktrace.c -ldl -o liblocktrace.so

# Build test program
gcc wrapper/banking_deadlock.c -pthread -o banking_deadlock
```

### Run Demo
```bash
# Run program with wrapper (will hang due to deadlock)
LD_PRELOAD=./liblocktrace.so timeout 3 ./banking_deadlock

# Analyze captured events
./build/bin/detector lock_events.bin
```

### Run Tests
```bash
make test
# Output: 148 tests passed
```

## 📂 Project Structure

```
live-deadlock-detector/
├── src/
│   ├── detector_main.c          # Main detector with DFS cycle detection
│   ├── collector/
│   │   └── transport_file.c     # Binary event reader
│   ├── state/
│   │   └── lock_state.c         # Lock ownership tracking
│   └── graph/
│       └── wait_for_graph.c     # Wait-for graph construction
├── wrapper/
│   ├── locktrace.c              # LD_PRELOAD wrapper
│   └── banking_deadlock.c       # Example deadlock program
├── scenarios/
│   ├── banking_deadlock.bin     # 2-thread deadlock
│   ├── three_thread_cycle.bin   # 3-thread cycle
│   └── clean_execution.bin      # No deadlock
├── tests/
│   ├── test_collector.c
│   ├── test_state.c
│   ├── test_graph.c
│   └── test_mock_pipeline.c
└── tools/
    └── generate_events.py       # Binary event generator

└── web/                         # (Coming soon: Web UI)
```

## 🧪 Test Scenarios

### 1. Banking Deadlock (2 threads)
```bash
./build/bin/detector scenarios/banking_deadlock.bin
# Result: DEADLOCK - Thread A ⇄ Thread B
```

### 2. Three-Thread Cycle
```bash
./build/bin/detector scenarios/three_thread_cycle.bin
# Result: DEADLOCK - Thread A → B → C → A
```

### 3. Clean Execution
```bash
./build/bin/detector scenarios/clean_execution.bin
# Result: No deadlock detected ✅
```

## 🎓 Educational Value

This project demonstrates:
- **System Programming**: LD_PRELOAD, dynamic linking, binary formats
- **Concurrency**: Mutex handling, deadlock scenarios
- **Graph Algorithms**: DFS for cycle detection
- **Testing**: Comprehensive unit and integration tests
- **Software Engineering**: Modular architecture, clear separation of concerns

## 🛠️ Technical Details

### Event Format (Binary)
```
LOCK_REQUEST:  40 bytes (header 28 + body 12)
LOCK_ACQUIRED: 36 bytes (header 28 + body 8)
LOCK_RELEASED: 28 bytes (header only)

Header: schema_version(1) + event_type(1) + pid(4) + tid(4) + 
        timestamp_ns(8) + lock_addr(8) + _pad[2](2)
```

### Detection Algorithm
1. Build wait-for graph from events
2. Run DFS from each thread
3. Detect back edges (cycles)
4. Report cycle path and recommendations

## 📊 Test Coverage

- ✅ Event collection (file transport)
- ✅ Lock state management (ownership, waiters)
- ✅ Wait-for graph (edge insertion, removal)
- ✅ Cycle detection (2-thread, 3-thread, N-thread)
- ✅ Edge cases (orphaned locks, thread exits, duplicate requests)
- ✅ Integration (end-to-end pipeline)

**Total: 148 tests, 100% pass rate**

## 👥 Team

Built by students for Operating Systems course end-semester project.

## 📝 License

Educational project - free to use and modify.

---

**Demo Ready! 🎉**
