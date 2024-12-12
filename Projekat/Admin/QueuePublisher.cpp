#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Queue.h"
#define INITIAL_CAPACITY 2         // Initial queue capacity
#define MAX_MESSAGE_SIZE 256       // Maximum message size

// Function to initialize the queue
void initQueue(Queue* queue) {
    // Initialize queue properties
    queue->capacity = INITIAL_CAPACITY;
    queue->idPublishers = (int*)malloc(queue->capacity * sizeof(int));
    if (queue->idPublishers == NULL) {
        printf("Memory allocation failed for idPublishers\n");
        return;  // Exit if memory allocation fails
    }

    queue->messages = (char**)malloc(queue->capacity * sizeof(char*));
    if (queue->messages == NULL) {
        printf("Memory allocation failed for messages\n");
        free(queue->idPublishers);  // Free previously allocated memory
        return;  // Exit if memory allocation fails
    }

    // Allocate memory for each message string
    for (int i = 0; i < queue->capacity; i++) {
        queue->messages[i] = (char*)malloc(MAX_MESSAGE_SIZE * sizeof(char));
        if (queue->messages[i] == NULL) {
            printf("Memory allocation failed for message %d\n", i);
            // Free previously allocated memory
            for (int j = 0; j < i; j++) {
                free(queue->messages[j]);
            }
            free(queue->messages);
            free(queue->idPublishers);
            return;
        }
    }

    queue->front = 0;
    queue->rear = -1;
    queue->size = 0;

    // Initialize semaphores
    queue->emptySemaphore = CreateSemaphore(NULL, INITIAL_CAPACITY, INITIAL_CAPACITY, NULL); //2 polje INITIAL_CAPACITY = 20 tj 20 slobodnih mesta imamo tj dozvoljva 20 podata u queue da dodaju threadovi
    queue->fullSemaphore = CreateSemaphore(NULL, 0, INITIAL_CAPACITY, NULL);                // 0 jer je semafor prvo prazan pa imamo 0 zauzetih mesta a max je do 20      kako dodajemo podatke tako dodajemo ovde     

    // Initialize critical section
    InitializeCriticalSection(&queue->criticalSection);
}

// Debugging print inside expandQueue
void expandQueue(Queue* queue) {
    EnterCriticalSection(&queue->criticalSection);  // Lock the critical section

    printf("Expanding queue: Current size = %d, Current capacity = %d\n", queue->size, queue->capacity);

    // Double the capacity
    int newCapacity = queue->capacity * 2;

    // Allocate new memory for the idPublishers and messages arrays
    int* newIdPublishers = (int*)malloc(newCapacity * sizeof(int));
    char** newMessages = (char**)malloc(newCapacity * sizeof(char*));

    if (newIdPublishers == NULL || newMessages == NULL) {
        printf("Memory allocation failed while expanding the queue\n");
        LeaveCriticalSection(&queue->criticalSection);  // Release the lock
        return;
    }

    // Copy old data to new arrays
    for (int i = 0; i < queue->size; i++) {
        int index = (queue->front + i) % queue->capacity;
        newIdPublishers[i] = queue->idPublishers[index];

        // Handle the case where the message might be NULL
        if (queue->messages[index] != NULL) {
            newMessages[i] = _strdup(queue->messages[index]);
            if (newMessages[i] == NULL) {
                printf("Memory allocation failed for new message during queue expansion\n");

                // Free already allocated memory before returning
                for (int j = 0; j < i; j++) {
                    free(newMessages[j]);
                }
                free(newMessages);
                free(newIdPublishers);
                LeaveCriticalSection(&queue->criticalSection);  // Release lock
                return;
            }
        }
        else {
            newMessages[i] = NULL;  // If the message is NULL, set the new entry to NULL as well
        }
    }

    // Free old arrays
    free(queue->idPublishers);
    free(queue->messages);

    // Assign new arrays to the queue
    queue->idPublishers = newIdPublishers;
    queue->messages = newMessages;

    // Update the front and rear positions
    queue->front = 0;
    queue->rear = queue->size - 1;
    queue->capacity = newCapacity;

    printf("Queue expanded: New capacity = %d\n", queue->capacity);

    // Release empty slots in the semaphore to reflect the new capacity
    ReleaseSemaphore(queue->emptySemaphore, newCapacity - queue->size, NULL);

    LeaveCriticalSection(&queue->criticalSection);  // Release critical section
}


void enqueue(Queue* queue, int idPublisher, const char* message) {
    printf("Enqueueing: Queue size = %d, Queue capacity = %d, Rear = %d\n", queue->size, queue->capacity, queue->rear);

    // Wait for an empty slot to become available
    WaitForSingleObject(queue->emptySemaphore, INFINITE);
    EnterCriticalSection(&queue->criticalSection);

    // If the queue is full, expand it
    if (queue->size == queue->capacity) {
        expandQueue(queue);
    }

    // Save the old rear index before incrementing it
    int oldRear = queue->rear;

    // Update the rear index
    queue->rear = (queue->rear + 1) % queue->capacity;

    // Free the message at the old rear position before overwriting it
    if (queue->size > 0 && queue->messages[oldRear] != NULL) {
        free(queue->messages[oldRear]);
    }

    // Allocate memory for the new message and copy it
    queue->messages[queue->rear] = _strdup(message);  // Copy the message

    // Store the publisher ID
    queue->idPublishers[queue->rear] = idPublisher;

    // Increase the size of the queue after adding the message
    queue->size++;

    // Signal that there is a new item in the queue
    ReleaseSemaphore(queue->fullSemaphore, 1, NULL);

    LeaveCriticalSection(&queue->criticalSection);

    printf("Message enqueued: Rear = %d, Size = %d\n", queue->rear, queue->size);
}





int dequeue(Queue* queue, int* idPublisher, char* message) {
    printf("Dequeuing: Front = %d, Size = %d\n", queue->front, queue->size);

    WaitForSingleObject(queue->fullSemaphore, INFINITE);
    EnterCriticalSection(&queue->criticalSection);

    if (queue->size == 0) {
        printf("Queue is empty. No messages to dequeue.\n");
        LeaveCriticalSection(&queue->criticalSection);
        return -1;  // Indicate an error if the queue is empty
    }

    *idPublisher = queue->idPublishers[queue->front];
    strcpy_s(message, MAX_MESSAGE_SIZE, queue->messages[queue->front]);

    free(queue->messages[queue->front]);

    queue->front = (queue->front + 1) % queue->capacity;
    queue->size--;  // Decrease the size after removing a message

    ReleaseSemaphore(queue->emptySemaphore, 1, NULL);
    LeaveCriticalSection(&queue->criticalSection);

    printf("Message dequeued: Front = %d, Size = %d\n", queue->front, queue->size);

    return 0;
}


// Function to check if the queue is empty
int isQueueEmpty(Queue* queue) {
    return (queue->size == 0);
}


// Function to free the memory allocated by the queue
void freeQueue(Queue* queue) {
    // Lock access to the queue using the critical section
    EnterCriticalSection(&queue->criticalSection);

    // Free the queue's publisher IDs and messages
    for (int i = 0; i < queue->size; i++) {
        free(queue->messages[i]);
    }
    free(queue->idPublishers);
    free(queue->messages);

    // Close semaphores
    CloseHandle(queue->emptySemaphore);
    CloseHandle(queue->fullSemaphore);

    // Delete the critical section
    DeleteCriticalSection(&queue->criticalSection);

    // Unlock the critical section (not necessary as we are freeing everything)
    LeaveCriticalSection(&queue->criticalSection);
}

// Function to print the elements of the queue
void printQueue(Queue* queue) {
    // Lock access to the queue using the critical section
    EnterCriticalSection(&queue->criticalSection);

    printf("Queue contents (front to rear):\n");
    for (int i = 0; i < queue->size; i++) {
        int index = (queue->front + i) % queue->capacity;
        printf("ID: %d, Message: %s\n", queue->idPublishers[index], queue->messages[index]);
    }

    // Unlock the critical section
    LeaveCriticalSection(&queue->criticalSection);
}
