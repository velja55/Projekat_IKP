#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Queue.h"
#define INITIAL_CAPACITY 10         // Initial queue capacity
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


// Function to expand the queue's capacity when it's full
void expandQueue(Queue* queue) {
    // Lock access to the queue using the critical section
    EnterCriticalSection(&queue->criticalSection);

    // Double the capacity of the queue
    int newCapacity = queue->capacity * 2;

    // Allocate new memory for the expanded queue
    int* newIdPublishers = (int*)malloc(newCapacity * sizeof(int));
    char** newMessages = (char**)malloc(newCapacity * sizeof(char*));

    // Copy old data to new arrays
    for (int i = 0; i < queue->size; i++) {
        int index = (queue->front + i) % queue->capacity;
        newIdPublishers[i] = queue->idPublishers[index];
        newMessages[i] = queue->messages[index];
    }

    // Free old arrays
    free(queue->idPublishers);
    free(queue->messages);

    // Update queue pointers and capacity
    queue->idPublishers = newIdPublishers;
    queue->messages = newMessages;
    queue->front = 0;
    queue->rear = queue->size - 1;
    queue->capacity = newCapacity;

    // Update the emptySemaphore to reflect new empty slots
    // The difference between the new capacity and the current size is the number of new empty slots
    ReleaseSemaphore(queue->emptySemaphore, newCapacity - queue->size, NULL);                           // npr imamo 4 zauzeta mesta emptySemaphore =0   slobodnih mesta
                                                                                                        //kada prosirujemo imacemo 8 -4 -> emptySemaphore =4 
    // Unlock the critical section
    LeaveCriticalSection(&queue->criticalSection);
}


// Function to add an element to the queue (enqueue)
void enqueue(Queue* queue, int idPublisher, const char* message) {
    // Wait for an empty slot in the queue
    WaitForSingleObject(queue->emptySemaphore, INFINITE);            //ovde se dekrementuje emptySemaphore  

    // Lock access to the queue using the critical section
    EnterCriticalSection(&queue->criticalSection);

    // Check if the queue is full (i.e., size == capacity)
    if (queue->size == queue->capacity) {
        // If full, expand the queue by calling expandQueue
        expandQueue(queue);
    }
    queue->rear = (queue->rear + 1) % queue->capacity;
    // Add the new publisher ID and message to the queue
    queue->idPublishers[queue->rear] = idPublisher;

    // Copy message and ensure it's not NULL
    if (queue->messages[queue->rear] != NULL) {
        free(queue->messages[queue->rear]);  // Free the old message if needed
    }
    queue->messages[queue->rear] = _strdup(message);

    queue->size++;

    // Release the fullSemaphore to signal that there is a new message in the queue
    ReleaseSemaphore(queue->fullSemaphore, 1, NULL);

    // Unlock the critical section
    LeaveCriticalSection(&queue->criticalSection);
}

// Function to remove an element from the queue (dequeue)
int dequeue(Queue* queue, int* idPublisher, char* message) {
    // Wait for a full slot (message available)
    WaitForSingleObject(queue->fullSemaphore, INFINITE);                //thredovi uzimaju iz queue i ovde se dekrementuje fullSemaphore

    // Lock access to the queue using the critical section
    EnterCriticalSection(&queue->criticalSection);

    // Remove the publisher ID and message from the front of the queue
    *idPublisher = queue->idPublishers[queue->front];
    strcpy_s(message,256, queue->messages[queue->front]);

    // Free the message string
    free(queue->messages[queue->front]);

    // Update the front pointer and size
    queue->front = (queue->front + 1) % queue->capacity;
    queue->size--;

    // Release the emptySemaphore to signal that a slot is now available
    ReleaseSemaphore(queue->emptySemaphore, 1, NULL);

    // Unlock the critical section
    LeaveCriticalSection(&queue->criticalSection);

    return 0;  // Indicating success
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
void printQueue( Queue* queue) {
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



