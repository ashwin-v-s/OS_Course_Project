/*
 * wrapper/locktrace.c
 * 
 * LD_PRELOAD wrapper that intercepts pthread_mutex_lock/unlock calls
 * and generates binary lock event files for analysis.
 * 
 * Usage:
 *   gcc -shared -fPIC -ldl locktrace.c -o liblocktrace.so
 *   LD_PRELOAD=./liblocktrace.so ./your_program
 *   ./detector lock_events.bin
 */

#define _GNU_SOURCE
#include <dlfcn.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>

/* Event types */
#define EVENT_LOCK_REQUEST  1
#define EVENT_LOCK_ACQUIRED 2
#define EVENT_LOCK_RELEASED 4

#define SCHED_FIFO 1

/* Output file */
static FILE *g_event_file = NULL;
static pthread_mutex_t g_file_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Original pthread functions */
static int (*real_pthread_mutex_lock)(pthread_mutex_t *) = NULL;
static int (*real_pthread_mutex_unlock)(pthread_mutex_t *) = NULL;
static int (*real_pthread_mutex_trylock)(pthread_mutex_t *) = NULL;

/* Get current thread ID */
static inline uint32_t get_tid(void) {
    return (uint32_t)syscall(SYS_gettid);
}

/* Get nanosecond timestamp */
static inline uint64_t get_timestamp_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

/* Initialize wrapper */
static void __attribute__((constructor)) init_wrapper(void) {
    /* Open output file */
    const char *output_file = getenv("LOCKTRACE_OUTPUT");
    if (!output_file) {
        output_file = "lock_events.bin";
    }
    
    g_event_file = fopen(output_file, "wb");
    if (!g_event_file) {
        fprintf(stderr, "[locktrace] Failed to open %s\n", output_file);
        return;
    }
    
    /* Get real pthread functions */
    real_pthread_mutex_lock = dlsym(RTLD_NEXT, "pthread_mutex_lock");
    real_pthread_mutex_unlock = dlsym(RTLD_NEXT, "pthread_mutex_unlock");
    real_pthread_mutex_trylock = dlsym(RTLD_NEXT, "pthread_mutex_trylock");
    
    if (!real_pthread_mutex_lock || !real_pthread_mutex_unlock) {
        fprintf(stderr, "[locktrace] Failed to load pthread functions\n");
        return;
    }
    
    fprintf(stderr, "[locktrace] Initialized. Writing to: %s\n", output_file);
}

/* Cleanup wrapper */
static void __attribute__((destructor)) cleanup_wrapper(void) {
    if (g_event_file) {
        fclose(g_event_file);
        fprintf(stderr, "[locktrace] Events written to file\n");
    }
}

/* Write event to file */
static void write_event(uint8_t event_type, uint32_t tid, uint64_t lock_addr, 
                        uint64_t timestamp_ns, int32_t priority, uint8_t is_trylock) {
    if (!g_event_file) return;
    
    /* Use real mutex to protect file access */
    if (real_pthread_mutex_lock) {
        real_pthread_mutex_lock(&g_file_mutex);
    }
    
    /* Write based on event type */
    if (event_type == EVENT_LOCK_REQUEST) {
        /* struct ldd_lock_request_event: 40 bytes */
        uint8_t buf[40];
        size_t offset = 0;
        
        /* Header: schema_version, event_type */
        buf[offset++] = 1;              /* schema_version */
        buf[offset++] = event_type;     /* event_type */
        
        /* Header: pid, tid */
        memcpy(buf + offset, &tid, 4); offset += 4;
        memcpy(buf + offset, &tid, 4); offset += 4;
        
        /* Header: timestamp_ns, lock_addr */
        memcpy(buf + offset, &timestamp_ns, 8); offset += 8;
        memcpy(buf + offset, &lock_addr, 8); offset += 8;
        
        /* Header: padding */
        buf[offset++] = 0;
        buf[offset++] = 0;
        
        /* Body: is_trylock */
        buf[offset++] = is_trylock;
        
        /* Body: padding */
        buf[offset++] = 0;
        
        /* Body: sched_policy, sched_priority */
        memcpy(buf + offset, &(int32_t){SCHED_FIFO}, 4); offset += 4;
        memcpy(buf + offset, &priority, 4); offset += 4;
        
        /* Body: padding */
        buf[offset++] = 0;
        buf[offset++] = 0;
        
        fwrite(buf, 1, 40, g_event_file);
    }
    else if (event_type == EVENT_LOCK_ACQUIRED) {
        /* struct ldd_lock_acquired_event: 36 bytes */
        uint8_t buf[36];
        size_t offset = 0;
        
        buf[offset++] = 1;
        buf[offset++] = event_type;
        memcpy(buf + offset, &tid, 4); offset += 4;
        memcpy(buf + offset, &tid, 4); offset += 4;
        memcpy(buf + offset, &timestamp_ns, 8); offset += 8;
        memcpy(buf + offset, &lock_addr, 8); offset += 8;
        buf[offset++] = 0; buf[offset++] = 0;  /* padding */
        memcpy(buf + offset, &(int32_t){SCHED_FIFO}, 4); offset += 4;
        memcpy(buf + offset, &priority, 4); offset += 4;
        
        fwrite(buf, 1, 36, g_event_file);
    }
    else if (event_type == EVENT_LOCK_RELEASED) {
        /* ldd_lock_released_event: 28 bytes (just header) */
        uint8_t buf[28];
        size_t offset = 0;
        
        buf[offset++] = 1;
        buf[offset++] = event_type;
        memcpy(buf + offset, &tid, 4); offset += 4;
        memcpy(buf + offset, &tid, 4); offset += 4;
        memcpy(buf + offset, &timestamp_ns, 8); offset += 8;
        memcpy(buf + offset, &lock_addr, 8); offset += 8;
        buf[offset++] = 0; buf[offset++] = 0;  /* padding */
        
        fwrite(buf, 1, 28, g_event_file);
    }
    
    fflush(g_event_file);
    
    if (real_pthread_mutex_unlock) {
        real_pthread_mutex_unlock(&g_file_mutex);
    }
}

/* Intercepted pthread_mutex_lock */
int pthread_mutex_lock(pthread_mutex_t *mutex) {
    if (!real_pthread_mutex_lock) {
        real_pthread_mutex_lock = dlsym(RTLD_NEXT, "pthread_mutex_lock");
    }
    
    uint32_t tid = get_tid();
    uint64_t lock_addr = (uint64_t)mutex;
    uint64_t timestamp = get_timestamp_ns();
    
    /* Log REQUEST event */
    write_event(EVENT_LOCK_REQUEST, tid, lock_addr, timestamp, 50, 0);
    
    /* Call real function */
    int result = real_pthread_mutex_lock(mutex);
    
    /* Log ACQUIRED event if successful */
    if (result == 0) {
        timestamp = get_timestamp_ns();
        write_event(EVENT_LOCK_ACQUIRED, tid, lock_addr, timestamp, 50, 0);
    }
    
    return result;
}

/* Intercepted pthread_mutex_unlock */
int pthread_mutex_unlock(pthread_mutex_t *mutex) {
    if (!real_pthread_mutex_unlock) {
        real_pthread_mutex_unlock = dlsym(RTLD_NEXT, "pthread_mutex_unlock");
    }
    
    uint32_t tid = get_tid();
    uint64_t lock_addr = (uint64_t)mutex;
    uint64_t timestamp = get_timestamp_ns();
    
    /* Log RELEASED event */
    write_event(EVENT_LOCK_RELEASED, tid, lock_addr, timestamp, 50, 0);
    
    /* Call real function */
    return real_pthread_mutex_unlock(mutex);
}

/* Intercepted pthread_mutex_trylock */
int pthread_mutex_trylock(pthread_mutex_t *mutex) {
    if (!real_pthread_mutex_trylock) {
        real_pthread_mutex_trylock = dlsym(RTLD_NEXT, "pthread_mutex_trylock");
    }
    
    uint32_t tid = get_tid();
    uint64_t lock_addr = (uint64_t)mutex;
    uint64_t timestamp = get_timestamp_ns();
    
    /* Log REQUEST event (trylock) */
    write_event(EVENT_LOCK_REQUEST, tid, lock_addr, timestamp, 50, 1);
    
    /* Call real function */
    int result = real_pthread_mutex_trylock(mutex);
    
    /* Log ACQUIRED event if successful */
    if (result == 0) {
        timestamp = get_timestamp_ns();
        write_event(EVENT_LOCK_ACQUIRED, tid, lock_addr, timestamp, 50, 0);
    }
    
    return result;
}
