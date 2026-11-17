/*
 * detector_main.c
 *
 * Main program for deadlock and priority inversion detection.
 * Reads binary event files and performs real-time analysis.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "collector/transport.h"
#include "collector/collector.h"
#include "common/events.h"
#include "common/graph.h"
#include "detection/deadlock_detector.h"

/* Simple statistics */
static int g_event_count = 0;
static int g_deadlock_detected = 0;

/* Forward declarations */
void print_usage(const char *prog_name);
void process_events(void);
void print_event(const ldd_typed_event_t *event, int event_type);

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }
    
    const char *scenario_file = argv[1];
    
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    printf("🔍 Live Deadlock Detector - Scenario Analysis\n");
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    printf("\n📂 Loading: %s\n\n", scenario_file);
    
    /* Initialize transport */
    if (ldd_transport_init(scenario_file) != 0) {
        fprintf(stderr, "❌ Failed to open scenario file: %s\n", scenario_file);
        return 1;
    }
    
    /* Process all events */
    process_events();
    
    /* Cleanup */
    ldd_transport_close();
    
    /* Print summary */
    printf("\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    printf("📊 Summary\n");
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    printf("Events processed: %d\n", g_event_count);
    
    if (g_deadlock_detected) {
        printf("Status: 🚨 DEADLOCK DETECTED\n");
    } else {
        printf("Status: ✅ No deadlocks detected\n");
    }
    
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    
    return g_deadlock_detected ? 1 : 0;
}

void print_usage(const char *prog_name) {
    printf("Usage: %s <scenario_file.bin>\n\n", prog_name);
    printf("Examples:\n");
    printf("  %s scenarios/scenario1_banking_deadlock.bin\n", prog_name);
    printf("  %s scenarios/scenario2_three_thread_cycle.bin\n", prog_name);
    printf("  %s scenarios/scenario3_priority_inversion.bin\n", prog_name);
    printf("\n");
}

void process_events(void) {
    ldd_raw_event_t raw;
    ldd_typed_event_t parsed;
    
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    printf("📋 Event Stream\n");
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n");
    
    while (1) {
        int rc = ldd_transport_read(&raw);
        
        if (rc == 1) {
            /* No more events */
            break;
        }
        
        if (rc < 0) {
            fprintf(stderr, "❌ Transport error\n");
            break;
        }
        
        /* Parse event */
        int event_type = ldd_collector_parse(&raw, &parsed);
        
        if (event_type <= 0) {
            fprintf(stderr, "❌ Parse error\n");
            continue;
        }
        
        g_event_count++;
        
        /* Print event */
        print_event(&parsed, event_type);
        
        /* Process event (updates state and graph) */
        ldd_collector_process(&parsed);
        
        /* Check for deadlock after processing this event */
        ldd_graph_snapshot_t *snap = ldd_graph_snapshot();
        if (snap) {
            ldd_deadlock_result_t result = ldd_detect_deadlock(snap);
            
            if (result.cycle_found) {
                printf("\n");
                printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
                printf("🚨 DEADLOCK DETECTED!\n");
                printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n");
                
                printf("📍 Cycle Path: ");
                for (int i = 0; i < result.cycle_len; i++) {
                    printf("Thread %u", result.cycle[i].waiting_tid);
                    if (i < result.cycle_len - 1) {
                        printf(" → ");
                    } else {
                        /* Show it loops back */
                        printf(" → Thread %u", result.cycle[0].waiting_tid);
                    }
                }
                printf("\n\n");
                
                /* Print details for each thread in cycle */
                for (int i = 0; i < result.cycle_len; i++) {
                    printf("┌─────────────────────────────────────────────┐\n");
                    printf("│ Thread %-37u│\n", result.cycle[i].waiting_tid);
                    printf("├─────────────────────────────────────────────┤\n");
                    printf("│ Status:  🔴 WAITING%-25s│\n", "");
                    printf("│ Owns:    Lock 0x%04llx%-23s│\n", 
                           (unsigned long long)result.cycle[i].lock_addr, "");
                    
                    /* Next thread in cycle is what this thread is waiting for */
                    int next = (i + 1) % result.cycle_len;
                    printf("│ Wants:   Lock 0x%04llx (held by Thread %u)%-3s│\n",
                           (unsigned long long)result.cycle[next].lock_addr,
                           result.cycle[i].owner_tid, "");
                    
                    printf("└─────────────────────────────────────────────┘\n");
                    if (i < result.cycle_len - 1) {
                        printf("\n");
                    }
                }
                
                printf("\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
                printf("💡 RECOMMENDATION\n");
                printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n");
                printf("To prevent this deadlock, enforce consistent lock ordering.\n");
                printf("All threads should acquire locks in the same sequence.\n\n");
                
                g_deadlock_detected = 1;
                ldd_graph_snapshot_free(snap);
                break;  /* Stop processing after detecting deadlock */
            }
            
            ldd_graph_snapshot_free(snap);
        }
    }
}

void print_event(const ldd_typed_event_t *event, int event_type) {
    const char *type_str;
    const char *icon;
    
    switch (event_type) {
        case EVENT_LOCK_REQUEST:
            icon = "🔵";
            type_str = "REQUEST";
            break;
        case EVENT_LOCK_ACQUIRED:
            icon = "🟢";
            type_str = "ACQUIRED";
            break;
        case EVENT_LOCK_RELEASED:
            icon = "⚪";
            type_str = "RELEASED";
            break;
        default:
            icon = "❓";
            type_str = "UNKNOWN";
            break;
    }
    
    printf("%s [%8llu μs] Thread %u: %-10s Lock 0x%04llx",
           icon,
           (unsigned long long)(event->hdr.timestamp_ns / 1000),
           event->hdr.tid,
           type_str,
           (unsigned long long)event->hdr.lock_addr);
    
    if (event_type == EVENT_LOCK_REQUEST || 
        event_type == EVENT_LOCK_ACQUIRED) {
        uint32_t priority = 0xFFFFFFFF;
        if (event_type == EVENT_LOCK_REQUEST) {
            priority = event->lock_request.sched_priority;
        } else {
            priority = event->lock_acquired.sched_priority;
        }
        if (priority != 0xFFFFFFFF) {
            printf(" (prio %u)", priority);
        }
    }
    
    printf("\n");
}
