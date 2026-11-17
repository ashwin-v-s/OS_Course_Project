/*
 * DEADLOCK CASE 1: Simple Two-Thread Deadlock (AB-BA Pattern)
 * Classic textbook example
 */

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

pthread_mutex_t lock_A = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lock_B = PTHREAD_MUTEX_INITIALIZER;

void* thread1_func(void* arg) {
    printf("[Thread 1] Starting...\n");
    
    printf("[Thread 1] Acquiring Lock A\n");
    pthread_mutex_lock(&lock_A);
    printf("[Thread 1] ✓ Got Lock A\n");
    
    sleep(1);  // Give Thread 2 time to get Lock B
    
    printf("[Thread 1] Trying to acquire Lock B...\n");
    pthread_mutex_lock(&lock_B);  // ❌ DEADLOCK! Thread 2 has Lock B
    printf("[Thread 1] ✓ Got Lock B\n");
    
    // This code never executes
    pthread_mutex_unlock(&lock_B);
    pthread_mutex_unlock(&lock_A);
    
    return NULL;
}

void* thread2_func(void* arg) {
    printf("[Thread 2] Starting...\n");
    
    printf("[Thread 2] Acquiring Lock B\n");
    pthread_mutex_lock(&lock_B);
    printf("[Thread 2] ✓ Got Lock B\n");
    
    sleep(1);  // Give Thread 1 time to get Lock A
    
    printf("[Thread 2] Trying to acquire Lock A...\n");
    pthread_mutex_lock(&lock_A);  // ❌ DEADLOCK! Thread 1 has Lock A
    printf("[Thread 2] ✓ Got Lock A\n");
    
    // This code never executes
    pthread_mutex_unlock(&lock_A);
    pthread_mutex_unlock(&lock_B);
    
    return NULL;
}

int main() {
    printf("=== CASE 1: Simple Two-Thread Deadlock ===\n");
    printf("Thread 1 locks: A then B\n");
    printf("Thread 2 locks: B then A\n");
    printf("Result: AB-BA pattern = DEADLOCK!\n\n");
    
    pthread_t t1, t2;
    
    pthread_create(&t1, NULL, thread1_func, NULL);
    pthread_create(&t2, NULL, thread2_func, NULL);
    
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    
    printf("Program finished (this never prints due to deadlock)\n");
    return 0;
}
