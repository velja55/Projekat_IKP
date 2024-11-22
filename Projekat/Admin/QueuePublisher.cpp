#include <stdio.h>
#include <stdlib.h>
#include "Queue.h"

void initQueue(Queue* queue) {
    queue->front = NULL;
    queue->rear = NULL;
    queue->size = 0;
}

void enqueue(Queue* queue, int idPublisher, int newMaxSize) {
    QueueNode* newNode = (QueueNode*)malloc(sizeof(QueueNode));
    if (newNode == NULL) {
        printf("Error: Memory allocation failed.\n");
        return;
    }
    newNode->idPublisher = idPublisher;
    newNode->newMaxSize = newMaxSize;
    newNode->next = NULL;
    if (queue->rear == NULL) {
        queue->front = newNode;
        queue->rear = newNode;
    }
    else {
        queue->rear->next = newNode;
        queue->rear = newNode;
    }
    queue->size++;
}

int dequeue(Queue* queue, int* idPublisher, int* newMaxSize) {
    if (queue->front == NULL) {
        printf("Error: Queue is empty.\n");
        return -1;
    }
    QueueNode* temp = queue->front;
    *idPublisher = temp->idPublisher;
    *newMaxSize = temp->newMaxSize;
    queue->front = queue->front->next;
    if (queue->front == NULL) {
        queue->rear = NULL;
    }
    free(temp);
    queue->size--;
    return 0;
}

int isQueueEmpty(Queue* queue) {
    return (queue->size == 0);
}

void freeQueue(Queue* queue) {
    while (!isQueueEmpty(queue)) {
        int idPublisher, newMaxSize;
        dequeue(queue, &idPublisher, &newMaxSize);
    }
}