/*
 * NO DEADLOCK: Correct Lock Ordering
 * Shows how consistent ordering prevents deadlock
 */

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

pthread_mutex_t lock_A = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lock_B = PTHREAD_MUTEX_INITIALIZER;

void* thread1_func(void* arg) {
    printf("[Thread 1] Starting...\n");
    
    // Always lock A first, then B
    printf("[Thread 1] Acquiring Lock A\n");
    pthread_mutex_lock(&lock_A);
    printf("[Thread 1] ✓ Got Lock A\n");
    
    sleep(1);
    
    printf("[Thread 1] Acquiring Lock B\n");
    pthread_mutex_lock(&lock_B);
    printf("[Thread 1] ✓ Got Lock B\n");
    
    printf("[Thread 1] ✅ Task complete! Releasing locks...\n");
    pthread_mutex_unlock(&lock_B);
    pthread_mutex_unlock(&lock_A);
    printf("[Thread 1] Done!\n");
    
    return NULL;
}

void* thread2_func(void* arg) {
    printf("[Thread 2] Starting...\n");
    
    // Also locks A first, then B (SAME ORDER!)
    printf("[Thread 2] Acquiring Lock A\n");
    pthread_mutex_lock(&lock_A);
    printf("[Thread 2] ✓ Got Lock A\n");
    
    sleep(1);
    
    printf("[Thread 2] Acquiring Lock B\n");
    pthread_mutex_lock(&lock_B);
    printf("[Thread 2] ✓ Got Lock B\n");
    
    printf("[Thread 2] ✅ Task complete! Releasing locks...\n");
    pthread_mutex_unlock(&lock_B);
    pthread_mutex_unlock(&lock_A);
    printf("[Thread 2] Done!\n");
    
    return NULL;
}

int main() {
    printf("=== NO DEADLOCK: Consistent Lock Ordering ===\n");
    printf("Thread 1 locks: A then B\n");
    printf("Thread 2 locks: A then B (SAME ORDER!)\n");
    printf("Result: No circular dependency = NO DEADLOCK! ✅\n\n");
    
    pthread_t t1, t2;
    
    pthread_create(&t1, NULL, thread1_func, NULL);
    pthread_create(&t2, NULL, thread2_func, NULL);
    
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    
    printf("\n✅ Program finished successfully! Both threads completed.\n");
    return 0;
}
