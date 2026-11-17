# 🎯 DEMO COMMANDS - Ready to Present!

## ✅ Everything is Working! Here's What to Run:

---

## **SCENARIO 1: Banking Deadlock** (Main Demo)

```bash
cd ~/live-deadlock-detector
./build/bin/detector scenarios/banking_deadlock.bin
```

**What happens:**
- Shows 6 lock events
- Thread 1001 acquires account1, wants account2
- Thread 1002 acquires account2, wants account1
- **🚨 DEADLOCK DETECTED!**
- Shows cycle: Thread 1001 → Thread 1002 → Thread 1001
- Provides recommendations

---

## **SCENARIO 2: Clean Execution** (No Deadlock)

```bash
./build/bin/detector scenarios/clean_execution.bin
```

**What happens:**
- Shows 12 lock events
- Both threads acquire locks in SAME order
- All locks released properly
- **✅ No deadlocks detected**

---

## **SCENARIO 3: Three-Thread Cycle**

```bash
./build/bin/detector scenarios/three_thread_cycle.bin
```

**What happens:**
- Three threads in circular wait
- Thread 3001 → 3002 → 3003 → 3001
- **🚨 DEADLOCK DETECTED!**

---

## **WHAT TO EXPLAIN:**

### **For Banking Deadlock:**

> "This represents a banking system with concurrent transfers:
> 
> - Thread 1001: Transferring from Account1 to Account2
>   - Locks Account1 ✓
>   - Wants Account2 (held by Thread 1002) ⏸️
> 
> - Thread 1002: Transferring from Account2 to Account1
>   - Locks Account2 ✓
>   - Wants Account1 (held by Thread 1001) ⏸️
> 
> Result: Both threads stuck waiting for each other = DEADLOCK!
> 
> Our system:
> 1. Processes each lock event
> 2. Builds wait-for graph
> 3. Runs DFS to find cycles
> 4. Detects the circular wait at event 6
> 5. Reports detailed information
> 6. Provides remediation advice"

---

### **For Clean Execution:**

> "This shows proper lock ordering:
> 
> Both threads acquire Lock 1 BEFORE Lock 2.
> Same order = No circular wait = No deadlock.
> 
> This demonstrates our system correctly identifies safe programs."

---

## **INPUT FORMAT:**

> "These binary files contain lock events in the exact format that eBPF would produce:
> 
> - Each event is a packed C struct
> - Contains: thread ID, lock address, timestamp, priority
> - Same format our system will read from live eBPF tracer
> 
> For this demo, we pre-generated these files to show the detection logic.
> Next phase: Connect live eBPF to capture events from running programs."

---

## **TECHNICAL DETAILS IF ASKED:**

### **Event Format:**
- LOCK_REQUEST: 40 bytes
- LOCK_ACQUIRED: 36 bytes  
- LOCK_RELEASED: 28 bytes

### **Detection Algorithm:**
- DFS-based cycle detection
- O(V + E) complexity
- V = threads, E = wait-for edges

### **Components:**
- Event Collector: Parses binary events
- State Manager: Tracks lock ownership
- Graph Builder: Maintains wait-for graph
- Deadlock Detector: Finds cycles using DFS

---

## **IF TEACHER ASKS: "Show me with a real program"**

**Answer:**
> "Currently demonstrating with pre-generated event traces. 
> The next phase is integrating the LD_PRELOAD wrapper which will:
> 
> 1. Intercept pthread_mutex_lock/unlock calls
> 2. Generate these same binary events at runtime
> 3. Feed them to our detector
> 
> Then we can analyze ANY C/C++ program you give us!
> 
> Would you like me to show the existing test suite? We have 148 passing tests covering all components."

---

## **BACKUP: Show Existing Tests**

```bash
make test
```

Shows:
- 28 tests: Event Collector ✅
- 42 tests: State Manager ✅
- 33 tests: Graph Builder ✅
- 45 tests: Integration ✅

**Total: 148 tests passing!**

---

## ✅ **YOU'RE READY FOR DEMO!**

Practice these 3 scenarios:
1. Banking deadlock (main)
2. Clean execution (contrast)
3. Three-thread cycle (complex)

**Time needed:** 10-15 minutes total

**Confidence level:** HIGH! 🎯🔥
