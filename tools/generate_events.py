#!/usr/bin/env python3
"""
Event Generator - Creates binary lock event files

Generates events in the exact format matching ldd_event structs from events.h
"""

import struct
import time

# Event types
EVENT_LOCK_REQUEST = 1
EVENT_LOCK_ACQUIRED = 2
EVENT_LOCK_RELEASED = 4

# Scheduling constants
SCHED_FIFO = 1
SCHED_OTHER = 0

def create_lock_request_event(tid, lock_addr, timestamp_ns, priority=50, is_trylock=0):
    """
    struct ldd_lock_request_event {
        struct ldd_event_hdr hdr;  // 28 bytes
        uint8_t  is_trylock;       // 1 byte
        uint8_t  _pad2[1];         // 1 byte
        int32_t  sched_policy;     // 4 bytes (signed)
        int32_t  sched_priority;   // 4 bytes (signed)
        uint8_t  _pad3[2];         // 2 bytes
    } __attribute__((packed));     // Total: 40 bytes
    
    Header layout:
        uint8_t  schema_version;   // 1 byte
        uint8_t  event_type;       // 1 byte
        uint32_t pid;              // 4 bytes
        uint32_t tid;              // 4 bytes
        uint64_t timestamp_ns;     // 8 bytes
        uint64_t lock_addr;        // 8 bytes
        uint8_t  _pad[2];          // 2 bytes
    """
    # Header: 1+1+4+4+8+8+2 = 28 bytes
    # Body: 1+1+4+4+2 = 12 bytes
    # Total: 40 bytes
    
    return struct.pack(
        '<BBIIQQxxBxiixx',  # Little-endian, packed
        1,                   # schema_version
        EVENT_LOCK_REQUEST,  # event_type
        tid,                 # pid
        tid,                 # tid
        timestamp_ns,        # timestamp_ns (Q = uint64)
        lock_addr,           # lock_addr (Q = uint64)
        # xx = 2 bytes padding (_pad)
        is_trylock,          # is_trylock
        # x = 1 byte padding (_pad2)
        SCHED_FIFO,          # sched_policy (i = int32)
        priority,            # sched_priority (i = int32)
        # xx = 2 bytes padding (_pad3)
    )

def create_lock_acquired_event(tid, lock_addr, timestamp_ns, priority=50):
    """
    struct ldd_lock_acquired_event {
        struct ldd_event_hdr hdr;  // 28 bytes
        int32_t  sched_policy;     // 4 bytes
        int32_t  sched_priority;   // 4 bytes
    } __attribute__((packed));     // Total: 36 bytes
    """
    return struct.pack(
        '<BBIIQQxxii',
        1,
        EVENT_LOCK_ACQUIRED,
        tid,
        tid,
        timestamp_ns,
        lock_addr,
        # xx = 2 bytes padding
        SCHED_FIFO,
        priority
    )

def create_lock_released_event(tid, lock_addr, timestamp_ns):
    """
    typedef struct ldd_event_hdr ldd_lock_released_event;
    Total: 28 bytes (just the header)
    """
    return struct.pack(
        '<BBIIQQxx',
        1,
        EVENT_LOCK_RELEASED,
        tid,
        tid,
        timestamp_ns,
        lock_addr
        # xx = 2 bytes padding
    )

# ============================================================================
# Scenario Generators
# ============================================================================

def generate_banking_deadlock():
    """
    Banking deadlock scenario:
    Thread 1: locks account1, then wants account2
    Thread 2: locks account2, then wants account1
    """
    events = []
    
    # Thread 1 acquires account1
    events.append(create_lock_request_event(1001, 0xAAAA, 1000000))
    events.append(create_lock_acquired_event(1001, 0xAAAA, 1001000))
    
    # Thread 2 acquires account2
    events.append(create_lock_request_event(1002, 0xBBBB, 1002000))
    events.append(create_lock_acquired_event(1002, 0xBBBB, 1003000))
    
    # Thread 1 wants account2 (held by Thread 2) - WAITING
    events.append(create_lock_request_event(1001, 0xBBBB, 1004000))
    
    # Thread 2 wants account1 (held by Thread 1) - DEADLOCK!
    events.append(create_lock_request_event(1002, 0xAAAA, 1005000))
    
    return events

def generate_clean_execution():
    """No deadlock - proper lock ordering"""
    events = []
    
    # Thread 1: acquires A, then B, releases both
    events.append(create_lock_request_event(2001, 0x1111, 2000000))
    events.append(create_lock_acquired_event(2001, 0x1111, 2001000))
    events.append(create_lock_request_event(2001, 0x2222, 2002000))
    events.append(create_lock_acquired_event(2001, 0x2222, 2003000))
    events.append(create_lock_released_event(2001, 0x2222, 2004000))
    events.append(create_lock_released_event(2001, 0x1111, 2005000))
    
    # Thread 2: same order - no deadlock
    events.append(create_lock_request_event(2002, 0x1111, 2006000))
    events.append(create_lock_acquired_event(2002, 0x1111, 2007000))
    events.append(create_lock_request_event(2002, 0x2222, 2008000))
    events.append(create_lock_acquired_event(2002, 0x2222, 2009000))
    events.append(create_lock_released_event(2002, 0x2222, 2010000))
    events.append(create_lock_released_event(2002, 0x1111, 2011000))
    
    return events

def generate_three_thread_cycle():
    """Three threads in circular wait"""
    events = []
    
    # Each thread acquires one lock
    events.append(create_lock_request_event(3001, 0xAAAA, 3000000))
    events.append(create_lock_acquired_event(3001, 0xAAAA, 3001000))
    
    events.append(create_lock_request_event(3002, 0xBBBB, 3002000))
    events.append(create_lock_acquired_event(3002, 0xBBBB, 3003000))
    
    events.append(create_lock_request_event(3003, 0xCCCC, 3004000))
    events.append(create_lock_acquired_event(3003, 0xCCCC, 3005000))
    
    # Now create the cycle
    events.append(create_lock_request_event(3001, 0xBBBB, 3006000))  # T1 wants T2's lock
    events.append(create_lock_request_event(3002, 0xCCCC, 3007000))  # T2 wants T3's lock
    events.append(create_lock_request_event(3003, 0xAAAA, 3008000))  # T3 wants T1's lock - CYCLE!
    
    return events

def write_scenario(filename, events):
    """Write events to binary file"""
    with open(filename, 'wb') as f:
        for event in events:
            f.write(event)
    
    total_bytes = sum(len(e) for e in events)
    print(f"✓ Created {filename}: {len(events)} events, {total_bytes} bytes")

# ============================================================================
# Main
# ============================================================================

if __name__ == '__main__':
    import os
    os.makedirs('scenarios', exist_ok=True)
    
    print("Generating binary event scenarios...\n")
    
    write_scenario('scenarios/banking_deadlock.bin', generate_banking_deadlock())
    write_scenario('scenarios/clean_execution.bin', generate_clean_execution())
    write_scenario('scenarios/three_thread_cycle.bin', generate_three_thread_cycle())
    
    print("\n✅ All scenarios generated!")
    print("\nTest with:")
    print("  ./detector scenarios/banking_deadlock.bin")
