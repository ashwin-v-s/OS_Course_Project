/*
 * banking_deadlock.c
 * 
 * Example program with deadlock - simulates banking transfers
 * 
 * Compile: gcc banking_deadlock.c -pthread -o banking_deadlock
 * Run with wrapper: LD_PRELOAD=./liblocktrace.so ./banking_deadlock
 */

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

pthread_mutex_t account1_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t account2_lock = PTHREAD_MUTEX_INITIALIZER;

void* transfer_1_to_2(void* arg) {
    printf("[Thread 1] Starting transfer from Account1 to Account2...\n");
    
    /* Lock account1 */
    printf("[Thread 1] Locking Account1...\n");
    pthread_mutex_lock(&account1_lock);
    printf("[Thread 1] Acquired Account1 lock\n");
    
    /* Simulate some processing */
    sleep(1);
    
    /* Try to lock account2 - will deadlock! */
    printf("[Thread 1] Trying to lock Account2...\n");
    pthread_mutex_lock(&account2_lock);  /* DEADLOCK HERE! */
    printf("[Thread 1] Acquired Account2 lock\n");
    
    /* Transfer logic (never reached) */
    printf("[Thread 1] Transferring $100...\n");
    
    pthread_mutex_unlock(&account2_lock);
    pthread_mutex_unlock(&account1_lock);
    
    printf("[Thread 1] Transfer complete\n");
    return NULL;
}

void* transfer_2_to_1(void* arg) {
    printf("[Thread 2] Starting transfer from Account2 to Account1...\n");
    
    /* Lock account2 */
    printf("[Thread 2] Locking Account2...\n");
    pthread_mutex_lock(&account2_lock);
    printf("[Thread 2] Acquired Account2 lock\n");
    
    /* Simulate some processing */
    sleep(1);
    
    /* Try to lock account1 - will deadlock! */
    printf("[Thread 2] Trying to lock Account1...\n");
    pthread_mutex_lock(&account1_lock);  /* DEADLOCK HERE! */
    printf("[Thread 2] Acquired Account1 lock\n");
    
    /* Transfer logic (never reached) */
    printf("[Thread 2] Transferring $50...\n");
    
    pthread_mutex_unlock(&account1_lock);
    pthread_mutex_unlock(&account2_lock);
    
    printf("[Thread 2] Transfer complete\n");
    return NULL;
}

int main() {
    pthread_t t1, t2;
    
    printf("=== Banking System Deadlock Demo ===\n");
    printf("Starting concurrent transfers...\n\n");
    
    pthread_create(&t1, NULL, transfer_1_to_2, NULL);
    pthread_create(&t2, NULL, transfer_2_to_1, NULL);
    
    /* Wait for threads - will hang forever due to deadlock */
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    
    printf("All transfers complete\n");
    return 0;
}
