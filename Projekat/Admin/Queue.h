#ifndef QUEUE_H
#define QUEUE_H

#include <stddef.h>

// Define the QueueNode structure
typedef struct QueueNode {
    int idPublisher;
    int newMaxSize;
    struct QueueNode* next;
} QueueNode;

// Define the Queue structure
typedef struct {
    QueueNode* front;
    QueueNode* rear;
    size_t size;
} Queue;

// Function declarations
void initQueue(Queue* queue);
void enqueue(Queue* queue, int idPublisher, int newMaxSize);
int dequeue(Queue* queue, int* idPublisher, int* newMaxSize);
int isQueueEmpty(Queue* queue);
void freeQueue(Queue* queue);

#endif // QUEUE_H
