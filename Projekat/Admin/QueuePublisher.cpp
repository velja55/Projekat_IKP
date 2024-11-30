#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Queue.h"
#define INITIAL_CAPACITY 20         // Initial queue capacity
#define MAX_MESSAGE_SIZE 256       // Maximum message size



// Function to initialize the queue
void initQueue(Queue* queue) {
    queue->capacity = INITIAL_CAPACITY;
    queue->idPublishers = (int*)malloc(queue->capacity * sizeof(int));
    queue->messages = (char**)malloc(queue->capacity * sizeof(char*));
    for (int i = 0; i < queue->capacity; i++) {
        queue->messages[i] = (char*)malloc(MAX_MESSAGE_SIZE * sizeof(char));
    }
    queue->front = 0;
    queue->rear = -1;
    queue->size = 0;
}

// Function to expand the queue's capacity when it's full
void expandQueue(Queue* queue) {
    int newCapacity = queue->capacity * 2;
    int* newIdPublishers = (int*)malloc(newCapacity * sizeof(int));
    char** newMessages = (char**)malloc(newCapacity * sizeof(char*));

    for (int i = 0; i < newCapacity; i++) {
        newMessages[i] = (char*)malloc(MAX_MESSAGE_SIZE * sizeof(char));
    }

    // Copy elements from old queue to new one
    for (int i = 0; i < queue->size; i++) {
        int index = (queue->front + i) % queue->capacity;
        newIdPublishers[i] = queue->idPublishers[index];
        snprintf(newMessages[i], MAX_MESSAGE_SIZE, "%s", queue->messages[index]);
    }

    // Free old memory
    free(queue->idPublishers);
    for (int i = 0; i < queue->capacity; i++) {
        free(queue->messages[i]);
    }
    free(queue->messages);

    // Update queue to use new memory
    queue->idPublishers = newIdPublishers;
    queue->messages = newMessages;
    queue->capacity = newCapacity;
    queue->front = 0;
    queue->rear = queue->size - 1;
}

// Function to add an element to the queue (enqueue)
void enqueue(Queue* queue, int idPublisher, const char* message) {
    if (queue->size == queue->capacity) {
        expandQueue(queue);  // Expand queue if it's full
    }

    queue->rear = (queue->rear + 1) % queue->capacity;
    queue->idPublishers[queue->rear] = idPublisher;

    // Safely copy the message using snprintf
    snprintf(queue->messages[queue->rear], MAX_MESSAGE_SIZE, "%s", message);

    queue->size++;
}

// Function to remove an element from the queue (dequeue)
int dequeue(Queue* queue, int* idPublisher, char* message) {
    if (queue->size == 0) {
        printf("Error: Queue is empty.\n");
        return -1;
    }

    *idPublisher = queue->idPublishers[queue->front];

    // Safely copy the message using snprintf
    snprintf(message, MAX_MESSAGE_SIZE, "%s", queue->messages[queue->front]);

    queue->front = (queue->front + 1) % queue->capacity;
    queue->size--;

    return 0;
}

// Function to check if the queue is empty
int isQueueEmpty(Queue* queue) {
    return (queue->size == 0);
}

// Function to free the memory allocated by the queue
void freeQueue(Queue* queue) {
    free(queue->idPublishers);
    for (int i = 0; i < queue->capacity; i++) {
        free(queue->messages[i]);
    }
    free(queue->messages);
}


// Function to print the elements of the queue
void printQueue(const Queue* queue) {
    if (queue->size == 0) {
        printf("Queue is empty.\n");
        return;
    }

    printf("Queue contents:\n");
    printf("-----------------------------\n");
    printf("| %-10s | %-20s |\n", "PublisherID", "Message");
    printf("-----------------------------\n");

    for (int i = 0; i < queue->size; i++) {
        int index = (queue->front + i) % queue->capacity;
        printf("| %-10d | %-20s |\n", queue->idPublishers[index], queue->messages[index]);
    }

    printf("-----------------------------\n");
}


