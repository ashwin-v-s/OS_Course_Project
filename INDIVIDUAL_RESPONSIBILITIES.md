# 👥 **INDIVIDUAL RESPONSIBILITIES - WHAT EACH PERSON EXPLAINS**

**If Ma'am asks: "What did YOU specifically do?"**

Each member should be able to explain THEIR component in detail.

---

## 👤 **PERSON 1: EVENT COLLECTION & BINARY PROTOCOL**

### **Your Component:**
**"Ma'am, I handled the EVENT COLLECTION and BINARY PROTOCOL system."**

### **What to Study & Explain:**

#### **1. Binary Event Schema (28-byte format)**
**File:** `src/common/events.h`

**Know this:**
```c
typedef struct __attribute__((packed)) {
    uint8_t  schema_version;  // Always 1
    uint8_t  event_type;      // 1=REQ, 2=ACQ, 3=FAIL, 4=REL, 5=EXIT
    uint32_t pid;             // Process ID
    uint32_t tid;             // Thread ID
    uint64_t timestamp_ns;    // Nanosecond timestamp
    uint64_t lock_addr;       // Memory address of mutex
    uint8_t  _pad[2];         // Padding to 28 bytes
} ldd_event_header_t;
```

**Explain:**
- "Ma'am, I designed a compact 28-byte binary format to minimize overhead"
- "Uses `__attribute__((packed))` to prevent compiler padding"
- "5 event types: LOCK_REQUEST, LOCK_ACQUIRED, LOCK_FAILED, LOCK_RELEASED, THREAD_EXIT"

#### **2. Event Collector**
**Files:** 
- `src/collector/collector.c` (main logic)
- `src/collector/transport_file.c` (file I/O)

**Know this:**
- **Transport layer:** Reads raw bytes from `lock_events.bin`
- **Collector:** Validates schema version, parses into typed events
- **Framing:** Ensures complete 28-byte chunks (no partial reads)

**Explain:**
- "Ma'am, the collector reads binary events from the file"
- "First validates schema version to ensure compatibility"
- "Converts raw bytes into strongly-typed event structures"
- "Handles incomplete reads and EOF gracefully"

#### **3. Your Viva Questions:**

**Q: Why binary format instead of text?**
> "Ma'am, binary is much faster - no string formatting overhead in the critical path. Text would slow down the target program."

**Q: Why 28 bytes specifically?**
> "Ma'am, it's the minimum size needed: 1+1+4+4+8+8+2 = 28 bytes. Aligned for efficient memory access."

**Q: What if file is corrupted?**
> "Ma'am, collector validates schema version and checks minimum payload length. Rejects malformed events."

---

## 👤 **PERSON 2: LOCK STATE TRACKING & OWNERSHIP**

### **Your Component:**
**"Ma'am, I handled the LOCK STATE MANAGEMENT system."**

### **What to Study & Explain:**

#### **1. Lock State Manager**
**File:** `src/state/lock_state.c`

**Know this:**
```c
typedef struct {
    uint64_t lock_addr;     // Address of mutex
    uint32_t owner_tid;     // Thread that owns it (0 = free)
    uint32_t waiters[16];   // Threads waiting for this lock
    uint8_t  waiter_count;  // Number of waiters
} ldd_lock_state_t;
```

**Handles:**
- **LOCK_REQUEST:** If lock has owner, add requester to waiters list
- **LOCK_ACQUIRED:** Set owner, remove from waiters
- **LOCK_RELEASED:** Clear owner, update waiters
- **THREAD_EXIT:** Clean up all locks owned by exiting thread

**Explain:**
- "Ma'am, I track which thread owns which lock at any moment"
- "Maintain a lock table (array of up to 256 locks)"
- "When thread requests locked mutex, it's added to waiters list"
- "When thread exits, I clean up orphaned locks to prevent false positives"

#### **2. Re-entrancy Handling**

**Know this:**
- Tracks if same thread locks same mutex multiple times
- Counts recursive depth
- Only releases when depth reaches zero

**Explain:**
- "Ma'am, pthread mutexes support re-entrancy (PTHREAD_MUTEX_RECURSIVE)"
- "I count how many times same thread locks same mutex"
- "Only consider it free when unlock count matches lock count"

#### **3. Your Viva Questions:**

**Q: What if two threads lock same mutex?**
> "Ma'am, only one can own it. The second thread is added to the waiters list. This creates a potential wait-for edge."

**Q: How do you handle thread exit?**
> "Ma'am, THREAD_EXIT event triggers cleanup. I iterate through all locks, if this thread owns any, I release them to prevent stale ownership."

**Q: What's the maximum number of locks?**
> "Ma'am, currently 256 locks (configurable). Real programs rarely use more than 10-20 mutexes."

---

## 👤 **PERSON 3: WAIT-FOR GRAPH CONSTRUCTION**

### **Your Component:**
**"Ma'am, I handled the WAIT-FOR GRAPH construction."**

### **What to Study & Explain:**

#### **1. Graph Structure**
**File:** `src/graph/graph.c`

**Know this:**
```c
typedef struct {
    uint32_t from_tid;      // Waiting thread
    uint32_t to_tid;        // Thread being waited for
    uint64_t lock_addr;     // Lock they're fighting over
} ldd_graph_edge_t;
```

**Graph represents:**
- **Nodes:** Threads
- **Edges:** Dependencies (A waits for lock held by B → edge A→B)

**Explain:**
- "Ma'am, I build a directed graph where edges represent wait-for relationships"
- "If Thread A wants a lock held by Thread B, I create edge A→B"
- "When Thread A acquires the lock, I remove edge A→B"

#### **2. Edge Management**

**Operations:**
- **Add Edge:** When LOCK_REQUEST and lock has owner
- **Remove Edge:** When LOCK_ACQUIRED or LOCK_RELEASED
- **Update Edge:** When ownership changes

**Explain:**
- "Ma'am, edges are dynamic - they appear when threads start waiting and disappear when they acquire locks"
- "I maintain an adjacency list representation for efficient traversal"

#### **3. Snapshot Isolation**

**Know this:**
```c
ldd_graph_snapshot_t* ldd_graph_snapshot(ldd_graph_t* graph);
```

**Why snapshots?**
- Detection algorithm needs consistent view
- Active graph keeps changing
- Snapshot is immutable copy

**Explain:**
- "Ma'am, I create immutable snapshots for analysis"
- "This ensures DFS runs on consistent state without holding locks during traversal"
- "Snapshot isolation prevents race conditions"

#### **4. Your Viva Questions:**

**Q: Why use a graph?**
> "Ma'am, deadlock is fundamentally a graph problem - circular wait means cycle in wait-for graph. Graph makes detection straightforward."

**Q: How do you handle multiple edges?**
> "Ma'am, a thread can wait for multiple locks, creating multiple outgoing edges. But typically each thread waits for only one lock at a time."

**Q: What if graph becomes huge?**
> "Ma'am, graph size is bounded by number of threads (usually 10-50). Even with 100 threads, graph is small enough to fit in memory."

---

## 👤 **PERSON 4: DEADLOCK DETECTION ALGORITHM**

### **Your Component:**
**"Ma'am, I handled the DEADLOCK DETECTION using DFS algorithm."**

### **What to Study & Explain:**

#### **1. 3-Color DFS Algorithm**
**File:** `src/detection/deadlock_detector.c`

**Know this:**
```c
typedef enum {
    DFS_UNVISITED = 0,  // White - not yet visited
    DFS_VISITING = 1,   // Gray - currently on stack
    DFS_VISITED = 2     // Black - completely explored
} dfs_state_t;
```

**Algorithm:**
1. Mark all threads as WHITE (unvisited)
2. For each unvisited thread, start DFS
3. Mark current thread GRAY (visiting)
4. Follow outgoing edge (who is this thread waiting for?)
5. If target is GRAY → **BACK-EDGE FOUND = CYCLE = DEADLOCK!**
6. If target is WHITE → recursively visit
7. Mark thread BLACK (visited) when all branches explored

**Explain:**
- "Ma'am, I use 3-color DFS to detect cycles"
- "WHITE means unvisited, GRAY means on current path, BLACK means fully explored"
- "Encountering a GRAY node proves we've reached an ancestor in current traversal - that's a back-edge, which means cycle"

#### **2. Cycle Reconstruction**

**Know this:**
```c
ldd_cycle_t* reconstruct_cycle(uint32_t cycle_start_tid, ...);
```

**What it does:**
- Traces back from detected cycle
- Builds complete path: Thread A → B → C → A
- Records which locks are involved

**Explain:**
- "Ma'am, once cycle is detected, I reconstruct the exact path"
- "I trace backwards from the back-edge to show complete circular dependency"
- "Example: Thread 458 → 459 → 458 (2-thread cycle)"

#### **3. Time Complexity**

**Know this:**
- **O(V + E)** where V = threads, E = wait-for edges
- Typically V = 2-50, E = 1-10
- Detection takes microseconds

**Explain:**
- "Ma'am, DFS is O(V+E) - linear in graph size"
- "Even with 100 threads and 50 dependencies, detection is under 1 millisecond"

#### **4. Your Viva Questions:**

**Q: Why 3 colors instead of 2?**
> "Ma'am, 2 colors (visited/unvisited) cannot distinguish between cross-edges in a DAG and back-edges that form cycles. GRAY identifies nodes currently on recursion stack."

**Q: What if there are multiple cycles?**
> "Ma'am, I detect one cycle at a time. First detected cycle is reported. Multiple disjoint cycles are rare in practice."

**Q: Can DFS miss deadlocks?**
> "Ma'am, no - if a cycle exists, DFS guarantees to find it. It's a complete algorithm for cycle detection in directed graphs."

**Q: Why not BFS?**
> "Ma'am, DFS is better for cycle detection because it naturally tracks the recursion path. BFS would need extra bookkeeping to remember paths."

---

## 📊 **SUMMARY TABLE - WHO STUDIES WHAT:**

| Person | Component | Files to Study | Key Concept |
|--------|-----------|----------------|-------------|
| **Person 1** | Event Collection | `events.h`, `collector.c`, `transport_file.c` | Binary protocol, 28-byte schema |
| **Person 2** | Lock State | `lock_state.c` | Ownership tracking, waiters list |
| **Person 3** | Graph | `graph.c` | Wait-for edges, snapshot isolation |
| **Person 4** | Detection | `deadlock_detector.c` | 3-color DFS, cycle detection |

---

## 🎯 **ADDITIONAL COMPONENTS (SHARED UNDERSTANDING):**

### **All 4 should understand these:**

#### **A. LD_PRELOAD Wrapper** (Person 2 leads, all should know basics)
**File:** `wrapper/locktrace.c`
- How it intercepts pthread_mutex_lock/unlock
- `dlsym(RTLD_NEXT)` mechanism
- How it logs events

#### **B. Web Interface** (Person 2 leads, all should know workflow)
**Files:** `web/simple_server.py`, `web/templates/index.html`
- Upload → Compile → Run → Analyze flow
- How backend calls detector binary

#### **C. Lock-Order Advisory** (Person 4 leads, all should know concept)
**File:** `src/remediation/lock_order_advisor.c`
- How it sorts lock addresses
- Why consistent ordering prevents deadlock

---

## 💬 **DEMO DAY - EACH PERSON'S 30-SECOND PITCH:**

### **Person 1:**
> "Ma'am, I built the event collection system. When the wrapper logs a lock operation, it writes a compact 28-byte binary event. My collector reads these events from file, validates the schema, and parses them into typed structures that other components can use."

### **Person 2:**
> "Ma'am, I built the lock state manager. As events flow in, I track which thread owns which lock and who's waiting. I maintain a lock table and waiters list. When a thread exits, I clean up orphaned locks to prevent false positives."

### **Person 3:**
> "Ma'am, I built the wait-for graph. Using lock state information, I create directed edges: if Thread A waits for lock held by B, I add edge A→B. I provide snapshot isolation so the detection algorithm sees a consistent view."

### **Person 4:**
> "Ma'am, I built the deadlock detector using 3-color DFS. I traverse the wait-for graph, marking threads as White, Gray, or Black. When I encounter a Gray node, that's a back-edge - proof of a cycle - meaning deadlock detected."

---

## 📚 **WHAT EACH PERSON SHOULD PRINT & STUDY:**

### **Person 1:**
```bash
Print these sections:
- events.h (lines 1-80)
- collector.c (lines 1-150)
- Binary format explanation
```

### **Person 2:**
```bash
Print these sections:
- lock_state.c (lines 1-200)
- Lock ownership diagram
- Re-entrancy handling
```

### **Person 3:**
```bash
Print these sections:
- graph.c (lines 1-180)
- Graph edge representation
- Snapshot isolation explanation
```

### **Person 4:**
```bash
Print these sections:
- deadlock_detector.c (lines 1-250)
- 3-color DFS algorithm
- Cycle reconstruction logic
```

---

## 🎓 **STUDY STRATEGY:**

### **Each person should:**
1. **Read their component's code** (1 hour)
2. **Understand data structures** (30 min)
3. **Trace through an example** (30 min)
4. **Practice explaining** out loud (30 min)
5. **Know 3-4 viva questions** (30 min)

**Total study time per person: ~3 hours**

---

## ⚡ **QUICK REFERENCE FOR EACH PERSON:**

### **Person 1 - EVENT COLLECTION**
- 28-byte packed struct
- 5 event types
- Binary for speed
- Validates schema

### **Person 2 - LOCK STATE**
- Lock table (256 max)
- Owner + waiters list
- Re-entrancy counting
- Thread exit cleanup

### **Person 3 - GRAPH**
- Directed edges A→B
- Dynamic (add/remove)
- Snapshot isolation
- Adjacency list

### **Person 4 - DETECTION**
- 3-color DFS
- WHITE-GRAY-BLACK
- Gray = back-edge = cycle
- O(V+E) complexity

---

# 🎯 **SHARE THIS WITH YOUR GROUP!**

**Each person:**
1. Read ONLY your section above
2. Study your assigned files
3. Practice your 30-second pitch
4. Learn your viva questions

**Result:**
- ✅ Each person is expert in their component
- ✅ All 4 can explain different parts
- ✅ Teacher sees clear division of work
- ✅ Everyone can answer deep questions on their part

---

# 💪 **YOU'RE READY TO DEFEND INDIVIDUAL CONTRIBUTIONS!**
