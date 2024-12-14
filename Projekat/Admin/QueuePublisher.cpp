#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Queue.h"
#define INITIAL_CAPACITY 2
// Initial queue capacity
#define MAX_MESSAGE_SIZE 256       // Maximum message size

#include "globalVariable.h";

// Funkcija za inicijalizaciju reda
void initQueue(Queue* queue) {
    queue->capacity = INITIAL_CAPACITY;
    queue->idPublishers = (int*)malloc(queue->capacity * sizeof(int));
    if (!queue->idPublishers) {
        printf("Memory allocation failed for idPublishers\n");
        exit(EXIT_FAILURE);
    }

    queue->messages = (char**)malloc(queue->capacity * sizeof(char*));
    if (!queue->messages) {
        free(queue->idPublishers);
        printf("Memory allocation failed for messages\n");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < queue->capacity; i++) {
        queue->messages[i] = (char*)malloc(MAX_MESSAGE_SIZE);
        if (!queue->messages[i]) {
            for (int j = 0; j < i; j++) {
                free(queue->messages[j]);
            }
            free(queue->messages);
            free(queue->idPublishers);
            printf("Memory allocation failed for message[%d]\n", i);
            exit(EXIT_FAILURE);
        }
    }

    queue->front = 0;
    queue->rear = -1;
    queue->size = 0;

    queue->emptySemaphore = CreateSemaphore(NULL, INITIAL_CAPACITY, INITIAL_CAPACITY, NULL);
    queue->fullSemaphore = CreateSemaphore(NULL, 0, INITIAL_CAPACITY, NULL);

    InitializeCriticalSection(&queue->criticalSection);
}

// Funkcija za širenje kapaciteta reda
void expandQueue(Queue* queue) {
    EnterCriticalSection(&queue->criticalSection);

    int newCapacity = queue->capacity * 2;
    int* newIdPublishers = (int*)malloc(newCapacity * sizeof(int));
    char** newMessages = (char**)malloc(newCapacity * sizeof(char*));

    if (!newIdPublishers || !newMessages) {
        printf("Memory allocation failed while expanding the queue\n");
        LeaveCriticalSection(&queue->criticalSection);
        return;
    }

    for (int i = 0; i < queue->size; i++) {
        int index = (queue->front + i) % queue->capacity;
        newIdPublishers[i] = queue->idPublishers[index];
        newMessages[i] = _strdup(queue->messages[index]);
        if (!newMessages[i]) {
            printf("Memory allocation failed for new message\n");
            for (int j = 0; j < i; j++) {
                free(newMessages[j]);
            }
            free(newMessages);
            free(newIdPublishers);
            LeaveCriticalSection(&queue->criticalSection);
            return;
        }
    }

    free(queue->idPublishers);
    free(queue->messages);

    queue->idPublishers = newIdPublishers;
    queue->messages = newMessages;
    queue->front = 0;
    queue->rear = queue->size - 1;
    queue->capacity = newCapacity;

    ReleaseSemaphore(queue->emptySemaphore, newCapacity - queue->size, NULL);

    LeaveCriticalSection(&queue->criticalSection);
}

// Funkcija za dodavanje elemenata u red
void enqueue(Queue* queue, int idPublisher, const char* message) {
    WaitForSingleObject(queue->emptySemaphore, INFINITE);
    EnterCriticalSection(&queue->criticalSection);

    if (queue->size == queue->capacity) {
        expandQueue(queue);
    }

    queue->rear = (queue->rear + 1) % queue->capacity;
    free(queue->messages[queue->rear]);
    queue->messages[queue->rear] = _strdup(message);
    queue->idPublishers[queue->rear] = idPublisher;
    queue->size++;

    ReleaseSemaphore(queue->fullSemaphore, 1, NULL);
    LeaveCriticalSection(&queue->criticalSection);
}

// Funkcija za uklanjanje elemenata iz reda
int dequeue(Queue* queue, int* idPublisher, char* message) {
    if (WaitForSingleObject(queue->fullSemaphore, 1000) == WAIT_TIMEOUT) {
        return -1;
    }

    EnterCriticalSection(&queue->criticalSection);

    if (queue->size == 0) {
        LeaveCriticalSection(&queue->criticalSection);
        return -1;
    }

    *idPublisher = queue->idPublishers[queue->front];
    strcpy_s(message, MAX_MESSAGE_SIZE, queue->messages[queue->front]);
    free(queue->messages[queue->front]);
    queue->messages[queue->front] = NULL;

    queue->front = (queue->front + 1) % queue->capacity;
    queue->size--;

    ReleaseSemaphore(queue->emptySemaphore, 1, NULL);
    LeaveCriticalSection(&queue->criticalSection);

    return 0;
}

// Funkcija za oslobadjanje memorije reda
void freeQueue(Queue* queue) {
    EnterCriticalSection(&queue->criticalSection);

    for (int i = 0; i < queue->capacity; i++) {
        if (queue->messages[i] != NULL) {
            free(queue->messages[i]);
        }
    }

    free(queue->idPublishers);
    free(queue->messages);
    CloseHandle(queue->emptySemaphore);
    CloseHandle(queue->fullSemaphore);

    LeaveCriticalSection(&queue->criticalSection);
    DeleteCriticalSection(&queue->criticalSection);
}

// Funkcija za ispis sadržaja reda
void printQueue(Queue* queue) {
    EnterCriticalSection(&queue->criticalSection);
    printf("Queue contents:\n");
    for (int i = 0; i < queue->size; i++) {
        int index = (queue->front + i) % queue->capacity;
        printf("ID: %d, Message: %s\n", queue->idPublishers[index], queue->messages[index]);
    }
    LeaveCriticalSection(&queue->criticalSection);
}

// Provera da li je red prazan
int isQueueEmpty(Queue* queue) {
    return queue->size == 0;
}
