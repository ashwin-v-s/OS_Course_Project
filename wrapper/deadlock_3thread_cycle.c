/*
 * THREE-THREAD CYCLE DEADLOCK
 * Thread A → Thread B → Thread C → Thread A
 * Classic 3-lock, 3-thread circular dependency
 */

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

pthread_mutex_t lock_A = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lock_B = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lock_C = PTHREAD_MUTEX_INITIALIZER;

void* thread_A(void* arg) {
    printf("[Thread A] Starting...\n");
    
    // Thread A: Acquires Lock A
    printf("[Thread A] Acquiring Lock A\n");
    pthread_mutex_lock(&lock_A);
    printf("[Thread A] ✓ Got Lock A\n");
    
    sleep(1);  // Let others acquire their first locks
    
    // Thread A: Tries to acquire Lock B (held by Thread B)
    printf("[Thread A] Trying to acquire Lock B...\n");
    pthread_mutex_lock(&lock_B);  // ❌ DEADLOCK! Thread B has Lock B
    printf("[Thread A] ✓ Got Lock B\n");
    
    // Never executes
    pthread_mutex_unlock(&lock_B);
    pthread_mutex_unlock(&lock_A);
    
    return NULL;
}

void* thread_B(void* arg) {
    printf("[Thread B] Starting...\n");
    
    // Thread B: Acquires Lock B
    printf("[Thread B] Acquiring Lock B\n");
    pthread_mutex_lock(&lock_B);
    printf("[Thread B] ✓ Got Lock B\n");
    
    sleep(1);  // Let others acquire their first locks
    
    // Thread B: Tries to acquire Lock C (held by Thread C)
    printf("[Thread B] Trying to acquire Lock C...\n");
    pthread_mutex_lock(&lock_C);  // ❌ DEADLOCK! Thread C has Lock C
    printf("[Thread B] ✓ Got Lock C\n");
    
    // Never executes
    pthread_mutex_unlock(&lock_C);
    pthread_mutex_unlock(&lock_B);
    
    return NULL;
}

void* thread_C(void* arg) {
    printf("[Thread C] Starting...\n");
    
    // Thread C: Acquires Lock C
    printf("[Thread C] Acquiring Lock C\n");
    pthread_mutex_lock(&lock_C);
    printf("[Thread C] ✓ Got Lock C\n");
    
    sleep(1);  // Let others acquire their first locks
    
    // Thread C: Tries to acquire Lock A (held by Thread A)
    printf("[Thread C] Trying to acquire Lock A...\n");
    pthread_mutex_lock(&lock_A);  // ❌ DEADLOCK! Thread A has Lock A
    printf("[Thread C] ✓ Got Lock A\n");
    
    // Never executes
    pthread_mutex_unlock(&lock_A);
    pthread_mutex_unlock(&lock_C);
    
    return NULL;
}

int main() {
    printf("=== THREE-THREAD CYCLE DEADLOCK ===\n");
    printf("3 Threads, 3 Locks\n");
    printf("\n");
    printf("Thread A: Lock A → Lock B\n");
    printf("Thread B: Lock B → Lock C\n");
    printf("Thread C: Lock C → Lock A\n");
    printf("\n");
    printf("Dependency Chain:\n");
    printf("  Thread A (has A, wants B) → Thread B\n");
    printf("  Thread B (has B, wants C) → Thread C\n");
    printf("  Thread C (has C, wants A) → Thread A\n");
    printf("\n");
    printf("Forms Cycle: A → B → C → A = DEADLOCK!\n");
    printf("═══════════════════════════════════════\n\n");
    
    pthread_t tA, tB, tC;
    
    // Create all three threads
    pthread_create(&tA, NULL, thread_A, NULL);
    pthread_create(&tB, NULL, thread_B, NULL);
    pthread_create(&tC, NULL, thread_C, NULL);
    
    // Wait for all (never returns)
    pthread_join(tA, NULL);
    pthread_join(tB, NULL);
    pthread_join(tC, NULL);
    
    printf("All threads finished (never prints)\n");
    return 0;
}
