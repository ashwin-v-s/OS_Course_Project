/*
 * DEADLOCK CASE 2: Dining Philosophers Problem
 * Classic synchronization problem - circular wait
 */

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

#define NUM_PHILOSOPHERS 5

pthread_mutex_t forks[NUM_PHILOSOPHERS];

void* philosopher(void* arg) {
    int id = *(int*)arg;
    int left_fork = id;
    int right_fork = (id + 1) % NUM_PHILOSOPHERS;
    
    printf("[Philosopher %d] Thinking...\n", id);
    sleep(1);
    
    printf("[Philosopher %d] Hungry! Trying to pick up left fork %d\n", id, left_fork);
    pthread_mutex_lock(&forks[left_fork]);
    printf("[Philosopher %d] ✓ Picked up left fork %d\n", id, left_fork);
    
    sleep(1);  // Give others time to pick their left forks
    
    printf("[Philosopher %d] Trying to pick up right fork %d\n", id, right_fork);
    pthread_mutex_lock(&forks[right_fork]);  // ❌ DEADLOCK! Circular wait
    printf("[Philosopher %d] ✓ Picked up right fork %d\n", id, right_fork);
    
    // Never executes
    printf("[Philosopher %d] Eating...\n", id);
    pthread_mutex_unlock(&forks[right_fork]);
    pthread_mutex_unlock(&forks[left_fork]);
    
    return NULL;
}

int main() {
    printf("=== CASE 2: Dining Philosophers Deadlock ===\n");
    printf("5 philosophers, 5 forks\n");
    printf("Each picks left fork first, then right\n");
    printf("Result: All hold left fork, wait for right = CIRCULAR DEADLOCK!\n\n");
    
    pthread_t philosophers[NUM_PHILOSOPHERS];
    int ids[NUM_PHILOSOPHERS];
    
    // Initialize forks
    for (int i = 0; i < NUM_PHILOSOPHERS; i++) {
        pthread_mutex_init(&forks[i], NULL);
    }
    
    // Create philosopher threads
    for (int i = 0; i < NUM_PHILOSOPHERS; i++) {
        ids[i] = i;
        pthread_create(&philosophers[i], NULL, philosopher, &ids[i]);
    }
    
    // Wait for all
    for (int i = 0; i < NUM_PHILOSOPHERS; i++) {
        pthread_join(philosophers[i], NULL);
    }
    
    printf("Dinner finished (never prints)\n");
    return 0;
}
