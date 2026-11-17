/*
 * DEADLOCK CASE 4: Nested Lock Acquisition Deadlock
 * Deep nesting with multiple locks
 */

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

pthread_mutex_t lock1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lock2 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lock3 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lock4 = PTHREAD_MUTEX_INITIALIZER;

void* worker1(void* arg) {
    printf("[Worker 1] Starting nested lock acquisition...\n");
    
    pthread_mutex_lock(&lock1);
    printf("[Worker 1] Level 1: ✓ Got Lock1\n");
    sleep(1);
    
    pthread_mutex_lock(&lock2);
    printf("[Worker 1] Level 2: ✓ Got Lock2\n");
    sleep(1);
    
    printf("[Worker 1] Level 3: Trying Lock3...\n");
    pthread_mutex_lock(&lock3);  // ❌ Worker 2 has Lock3
    printf("[Worker 1] Level 3: ✓ Got Lock3\n");
    
    pthread_mutex_lock(&lock4);
    printf("[Worker 1] Level 4: ✓ Got Lock4 - Task complete!\n");
    
    pthread_mutex_unlock(&lock4);
    pthread_mutex_unlock(&lock3);
    pthread_mutex_unlock(&lock2);
    pthread_mutex_unlock(&lock1);
    
    return NULL;
}

void* worker2(void* arg) {
    printf("[Worker 2] Starting nested lock acquisition...\n");
    
    pthread_mutex_lock(&lock3);
    printf("[Worker 2] Level 1: ✓ Got Lock3\n");
    sleep(1);
    
    pthread_mutex_lock(&lock4);
    printf("[Worker 2] Level 2: ✓ Got Lock4\n");
    sleep(1);
    
    printf("[Worker 2] Level 3: Trying Lock1...\n");
    pthread_mutex_lock(&lock1);  // ❌ Worker 1 has Lock1
    printf("[Worker 2] Level 3: ✓ Got Lock1\n");
    
    pthread_mutex_lock(&lock2);
    printf("[Worker 2] Level 4: ✓ Got Lock2 - Task complete!\n");
    
    pthread_mutex_unlock(&lock2);
    pthread_mutex_unlock(&lock1);
    pthread_mutex_unlock(&lock4);
    pthread_mutex_unlock(&lock3);
    
    return NULL;
}

int main() {
    printf("=== CASE 4: Nested Lock Acquisition Deadlock ===\n");
    printf("Worker 1: Lock1 → Lock2 → Lock3 → Lock4\n");
    printf("Worker 2: Lock3 → Lock4 → Lock1 → Lock2\n");
    printf("Result: Deep nesting creates circular wait = DEADLOCK!\n\n");
    
    pthread_t w1, w2;
    
    pthread_create(&w1, NULL, worker1, NULL);
    pthread_create(&w2, NULL, worker2, NULL);
    
    pthread_join(w1, NULL);
    pthread_join(w2, NULL);
    
    printf("Workers finished (never prints)\n");
    return 0;
}
