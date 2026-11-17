#!/usr/bin/env python3
"""
Binary Event Generator for Live Deadlock Detector

This script generates binary event files in the exact format that eBPF would produce.
Each event is a binary-packed structure matching the kernel event format.

Usage:
    python3 generate_scenario.py
"""

import struct
import os

# Event type constants (matching src/common/events.h)
EVENT_LOCK_REQUEST = 1
EVENT_LOCK_ACQUIRED = 2
EVENT_LOCK_RELEASED = 3

# Scheduling policy constants
SCHED_OTHER = 0
SCHED_FIFO = 1
SCHED_RR = 2
SCHED_UNAVAILABLE = 0xFFFFFFFF

# Priority constants
PRIO_UNAVAILABLE = 0xFFFFFFFF

def create_lock_request(tid, lock_addr, timestamp_ns, priority=50, sched_policy=SCHED_FIFO, is_trylock=0):
    """
    Create a LOCK_REQUEST event in binary format.
    
    Matches struct ldd_lock_request_event from events.h:
    - schema_version: uint8_t (1 byte)
    - event_type: uint8_t (1 byte)
    - pid: uint32_t (4 bytes)
    - tid: uint32_t (4 bytes)
    - timestamp_ns: uint64_t (8 bytes)
    - lock_addr: uint64_t (8 bytes)
    - is_trylock: uint8_t (1 byte)
    - padding: 3 bytes
    - sched_policy: uint32_t (4 bytes)
    - sched_priority: uint32_t (4 bytes)
    
    Total: 40 bytes
    """
    return struct.pack(
        '<BBIIQQB3xII',  # Little-endian format, 3x = 3 bytes padding
        1,                 # schema_version
        EVENT_LOCK_REQUEST,
        tid,               # pid (same as tid)
        tid,               # tid
        timestamp_ns,
        lock_addr,
        is_trylock,
        # 3x = 3 bytes padding
        sched_policy,
        priority
    )

def create_lock_acquired(tid, lock_addr, timestamp_ns, priority=50, sched_policy=SCHED_FIFO):
    """
    Create a LOCK_ACQUIRED event in binary format.
    
    Matches struct ldd_lock_acquired_event from events.h:
    - schema_version: uint8_t
    - event_type: uint8_t
    - pid: uint32_t
    - tid: uint32_t
    - timestamp_ns: uint64_t
    - lock_addr: uint64_t
    - sched_policy: uint32_t
    - sched_priority: uint32_t
    
    Total: 36 bytes
    """
    return struct.pack(
        '<BBIIQQII',
        1,
        EVENT_LOCK_ACQUIRED,
        tid,
        tid,
        timestamp_ns,
        lock_addr,
        sched_policy,
        priority
    )

def create_lock_released(tid, lock_addr, timestamp_ns):
    """
    Create a LOCK_RELEASED event in binary format.
    
    Matches ldd_lock_released_event (= ldd_event_hdr) from events.h:
    - schema_version: uint8_t
    - event_type: uint8_t
    - pid: uint32_t
    - tid: uint32_t
    - timestamp_ns: uint64_t  ← BEFORE lock_addr
    - lock_addr: uint64_t
    - _pad[2]: 2 bytes
    
    Total: 28 bytes
    """
    return struct.pack(
        '<BBIIqq2x',
        1,
        EVENT_LOCK_RELEASED,
        tid,
        tid,
        timestamp_ns,   # timestamp FIRST
        lock_addr       # lock_addr SECOND
        # 2x = 2 bytes padding
    )

def write_scenario_file(filename, events):
    """Write binary events to file."""
    os.makedirs(os.path.dirname(filename), exist_ok=True)
    with open(filename, 'wb') as f:
        for event in events:
            f.write(event)
    print(f"✓ Created: {filename} ({len(events)} events, {sum(len(e) for e in events)} bytes)")

# ============================================================================
# SCENARIO 1: Simple Two-Thread Deadlock (Banking)
# ============================================================================
def generate_scenario1_banking_deadlock():
    """
    Banking system deadlock:
    - Thread 1001: Transfer from Account1 to Account2
    - Thread 1002: Transfer from Account2 to Account1
    
    Timeline:
    1000 μs: T1001 requests Lock A (account1)
    1001 μs: T1001 acquires Lock A
    1002 μs: T1002 requests Lock B (account2)
    1003 μs: T1002 acquires Lock B
    1004 μs: T1001 requests Lock B (BLOCKED - T1002 has it)
    1005 μs: T1002 requests Lock A (BLOCKED - T1001 has it)
    -> DEADLOCK!
    """
    events = [
        create_lock_request(1001, 0xAAAA, 1000000, priority=50),
        create_lock_acquired(1001, 0xAAAA, 1001000, priority=50),
        create_lock_request(1002, 0xBBBB, 1002000, priority=50),
        create_lock_acquired(1002, 0xBBBB, 1003000, priority=50),
        create_lock_request(1001, 0xBBBB, 1004000, priority=50),  # T1 wants T2's lock
        create_lock_request(1002, 0xAAAA, 1005000, priority=50),  # T2 wants T1's lock -> DEADLOCK
    ]
    write_scenario_file('../scenarios/scenario1_banking_deadlock.bin', events)

# ============================================================================
# SCENARIO 2: Three-Thread Deadlock Cycle
# ============================================================================
def generate_scenario2_three_thread_cycle():
    """
    Three threads in circular wait:
    - Thread 2001 owns Lock X, wants Lock Y
    - Thread 2002 owns Lock Y, wants Lock Z
    - Thread 2003 owns Lock Z, wants Lock X
    
    Creates cycle: 2001 -> 2002 -> 2003 -> 2001
    """
    events = [
        # T2001 acquires Lock X
        create_lock_request(2001, 0xCCCC, 2000000),
        create_lock_acquired(2001, 0xCCCC, 2001000),
        
        # T2002 acquires Lock Y
        create_lock_request(2002, 0xDDDD, 2002000),
        create_lock_acquired(2002, 0xDDDD, 2003000),
        
        # T2003 acquires Lock Z
        create_lock_request(2003, 0xEEEE, 2004000),
        create_lock_acquired(2003, 0xEEEE, 2005000),
        
        # Now the circular wait begins
        create_lock_request(2001, 0xDDDD, 2006000),  # T2001 wants Y (T2002 has it)
        create_lock_request(2002, 0xEEEE, 2007000),  # T2002 wants Z (T2003 has it)
        create_lock_request(2003, 0xCCCC, 2008000),  # T2003 wants X (T2001 has it) -> DEADLOCK
    ]
    write_scenario_file('../scenarios/scenario2_three_thread_cycle.bin', events)

# ============================================================================
# SCENARIO 3: Priority Inversion
# ============================================================================
def generate_scenario3_priority_inversion():
    """
    Priority inversion scenario:
    - Thread 3001 (low priority 10) acquires Lock M
    - Thread 3002 (high priority 80) requests Lock M (BLOCKED by low-priority thread)
    
    This demonstrates priority inversion detection.
    """
    events = [
        # Low-priority thread acquires lock
        create_lock_request(3001, 0xFFFF, 3000000, priority=10),
        create_lock_acquired(3001, 0xFFFF, 3001000, priority=10),
        
        # High-priority thread wants the same lock
        create_lock_request(3002, 0xFFFF, 3002000, priority=80),  # PRIORITY INVERSION!
    ]
    write_scenario_file('../scenarios/scenario3_priority_inversion.bin', events)

# ============================================================================
# SCENARIO 4: No Deadlock (Clean Execution)
# ============================================================================
def generate_scenario4_no_deadlock():
    """
    Clean execution with proper lock ordering:
    - Thread 4001 acquires A then B, releases both
    - Thread 4002 acquires A then B, releases both
    
    No deadlock because both follow same lock order.
    """
    events = [
        # T4001: A -> B -> release B -> release A
        create_lock_request(4001, 0x1111, 4000000),
        create_lock_acquired(4001, 0x1111, 4001000),
        create_lock_request(4001, 0x2222, 4002000),
        create_lock_acquired(4001, 0x2222, 4003000),
        create_lock_released(4001, 0x2222, 4004000),
        create_lock_released(4001, 0x1111, 4005000),
        
        # T4002: A -> B -> release B -> release A (same order)
        create_lock_request(4002, 0x1111, 4006000),
        create_lock_acquired(4002, 0x1111, 4007000),
        create_lock_request(4002, 0x2222, 4008000),
        create_lock_acquired(4002, 0x2222, 4009000),
        create_lock_released(4002, 0x2222, 4010000),
        create_lock_released(4002, 0x1111, 4011000),
    ]
    write_scenario_file('../scenarios/scenario4_no_deadlock.bin', events)

# ============================================================================
# SCENARIO 5: Trylock Avoids Deadlock
# ============================================================================
def generate_scenario5_trylock_avoids_deadlock():
    """
    Thread 5002 uses trylock and backs off when lock is held:
    - Thread 5001 owns Lock A, wants Lock B
    - Thread 5002 owns Lock B, tries Lock A (trylock)
    - Trylock fails, T5002 backs off
    
    No deadlock because T5002 doesn't wait.
    """
    events = [
        # T5001 acquires A
        create_lock_request(5001, 0x3333, 5000000),
        create_lock_acquired(5001, 0x3333, 5001000),
        
        # T5002 acquires B
        create_lock_request(5002, 0x4444, 5002000),
        create_lock_acquired(5002, 0x4444, 5003000),
        
        # T5001 requests B (will wait)
        create_lock_request(5001, 0x4444, 5004000),
        
        # T5002 tries A (trylock, non-blocking)
        create_lock_request(5002, 0x3333, 5005000, is_trylock=1),
        # Trylock fails immediately, T5002 backs off
        create_lock_released(5002, 0x4444, 5006000),  # T5002 releases B
        
        # Now T5001 can acquire B
        create_lock_acquired(5001, 0x4444, 5007000),
        create_lock_released(5001, 0x4444, 5008000),
        create_lock_released(5001, 0x3333, 5009000),
    ]
    write_scenario_file('../scenarios/scenario5_trylock_avoids_deadlock.bin', events)

# ============================================================================
# MAIN
# ============================================================================
if __name__ == '__main__':
    print("Generating binary scenario files...\n")
    
    generate_scenario1_banking_deadlock()
    generate_scenario2_three_thread_cycle()
    generate_scenario3_priority_inversion()
    generate_scenario4_no_deadlock()
    generate_scenario5_trylock_avoids_deadlock()
    
    print("\n✅ All scenarios generated successfully!")
    print("\nScenario files created in: scenarios/")
    print("\nTo test, run:")
    print("  ./detector scenarios/scenario1_banking_deadlock.bin")
