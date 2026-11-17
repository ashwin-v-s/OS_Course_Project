/*
 * DEADLOCK CASE 3: Resource Allocation Graph Deadlock
 * Multiple resources, multiple processes
 */

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

pthread_mutex_t printer = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t scanner = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t disk = PTHREAD_MUTEX_INITIALIZER;

void* process_A(void* arg) {
    printf("[Process A] Need: Printer → Scanner → Disk\n");
    
    pthread_mutex_lock(&printer);
    printf("[Process A] ✓ Got Printer\n");
    sleep(1);
    
    printf("[Process A] Requesting Scanner...\n");
    pthread_mutex_lock(&scanner);  // ❌ Process B has Scanner
    printf("[Process A] ✓ Got Scanner\n");
    
    pthread_mutex_lock(&disk);
    printf("[Process A] ✓ Got Disk - Task complete!\n");
    
    pthread_mutex_unlock(&disk);
    pthread_mutex_unlock(&scanner);
    pthread_mutex_unlock(&printer);
    
    return NULL;
}

void* process_B(void* arg) {
    printf("[Process B] Need: Scanner → Disk → Printer\n");
    
    pthread_mutex_lock(&scanner);
    printf("[Process B] ✓ Got Scanner\n");
    sleep(1);
    
    printf("[Process B] Requesting Disk...\n");
    pthread_mutex_lock(&disk);  // ❌ Process C has Disk
    printf("[Process B] ✓ Got Disk\n");
    
    pthread_mutex_lock(&printer);
    printf("[Process B] ✓ Got Printer - Task complete!\n");
    
    pthread_mutex_unlock(&printer);
    pthread_mutex_unlock(&disk);
    pthread_mutex_unlock(&scanner);
    
    return NULL;
}

void* process_C(void* arg) {
    printf("[Process C] Need: Disk → Printer → Scanner\n");
    
    pthread_mutex_lock(&disk);
    printf("[Process C] ✓ Got Disk\n");
    sleep(1);
    
    printf("[Process C] Requesting Printer...\n");
    pthread_mutex_lock(&printer);  // ❌ Process A has Printer
    printf("[Process C] ✓ Got Printer\n");
    
    pthread_mutex_lock(&scanner);
    printf("[Process C] ✓ Got Scanner - Task complete!\n");
    
    pthread_mutex_unlock(&scanner);
    pthread_mutex_unlock(&printer);
    pthread_mutex_unlock(&disk);
    
    return NULL;
}

int main() {
    printf("=== CASE 3: Resource Allocation Deadlock ===\n");
    printf("3 Processes need 3 Resources\n");
    printf("Process A: Printer → Scanner → Disk\n");
    printf("Process B: Scanner → Disk → Printer\n");
    printf("Process C: Disk → Printer → Scanner\n");
    printf("Result: Circular dependency on resources = DEADLOCK!\n\n");
    
    pthread_t pA, pB, pC;
    
    pthread_create(&pA, NULL, process_A, NULL);
    pthread_create(&pB, NULL, process_B, NULL);
    pthread_create(&pC, NULL, process_C, NULL);
    
    pthread_join(pA, NULL);
    pthread_join(pB, NULL);
    pthread_join(pC, NULL);
    
    printf("All processes completed (never prints)\n");
    return 0;
}
