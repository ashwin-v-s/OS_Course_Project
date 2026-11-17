/*
 * src/collector/transport_file.c
 *
 * Transport layer implementation that reads events from binary files.
 * This replaces transport_mock.c for demo purposes, reading pre-generated
 * binary event files that match the exact format eBPF would produce.
 *
 * In production, this would be replaced with transport_fd.c that reads
 * from a real eBPF ring buffer.
 */

#include "transport.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

/* -------------------------------------------------------------------------
 * Internal state
 * ------------------------------------------------------------------------- */

static FILE *g_event_file = NULL;
static uint64_t g_lost_count = 0;

/* -------------------------------------------------------------------------
 * Public transport interface (declared in transport.h)
 * ------------------------------------------------------------------------- */

int ldd_transport_init(const char *filename) {
    if (g_event_file != NULL) {
        fclose(g_event_file);
    }
    
    g_event_file = fopen(filename, "rb");
    if (!g_event_file) {
        fprintf(stderr, "[transport] Failed to open %s\n", filename);
        return -1;
    }
    
    return 0;
}

int ldd_transport_read(ldd_raw_event_t *out) {
    if (!g_event_file || !out) {
        return -1;
    }
    
    /* Read schema_version and event_type first to determine event size */
    uint8_t header[2];
    size_t bytes_read = fread(header, 1, 2, g_event_file);
    
    if (bytes_read == 0) {
        if (feof(g_event_file)) {
            return 1;  /* No more events */
        }
        return -1;  /* Read error */
    }
    
    if (bytes_read != 2) {
        fprintf(stderr, "[transport] Incomplete header\n");
        return -1;
    }
    
    uint8_t schema_version = header[0];
    uint8_t event_type = header[1];
    
    /* Determine event size based on type */
    size_t event_size;
    switch (event_type) {
        case 1:  /* EVENT_LOCK_REQUEST */
            event_size = 40;
            break;
        case 2:  /* EVENT_LOCK_ACQUIRED */
            event_size = 36;
            break;
        case 3:  /* EVENT_LOCK_ACQUIRE_FAILED */
            event_size = 36;
            break;
        case 4:  /* EVENT_LOCK_RELEASED */
            event_size = 28;
            break;
        case 5:  /* EVENT_THREAD_EXIT */
            event_size = 36;
            break;
        default:
            fprintf(stderr, "[transport] Unknown event type: %u\n", event_type);
            return -1;
    }
    
    /* Copy header to output buffer */
    out->buf[0] = schema_version;
    out->buf[1] = event_type;
    
    /* Read rest of event */
    size_t remaining = event_size - 2;
    bytes_read = fread(&out->buf[2], 1, remaining, g_event_file);
    
    if (bytes_read != remaining) {
        fprintf(stderr, "[transport] Incomplete event: got %zu bytes, expected %zu\n", 
                bytes_read, remaining);
        return -1;
    }
    
    out->len = (uint16_t)event_size;
    return 0;  /* Success */
}

uint64_t ldd_transport_lost_count(void) {
    uint64_t count = g_lost_count;
    g_lost_count = 0;
    return count;
}

void ldd_transport_set_mock_source(int (*mock_read)(ldd_raw_event_t *)) {
    /* Not used in file transport, kept for API compatibility */
    (void)mock_read;
}

void ldd_transport_close(void) {
    if (g_event_file) {
        fclose(g_event_file);
        g_event_file = NULL;
    }
}

/* -------------------------------------------------------------------------
 * Test-only helpers (for compatibility with existing tests)
 * ------------------------------------------------------------------------- */

void ldd_mock_set_lost_count(uint64_t n) {
    g_lost_count = n;
}
