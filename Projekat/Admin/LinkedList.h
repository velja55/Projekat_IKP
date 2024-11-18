#ifndef LINKED_LIST_H
#define LINKED_LIST_H

#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>

// Structure to store key-value pairs (int as key, SOCKET as value)
typedef struct Node {
    int key;                // Key (ID of the subscriber)
    SOCKET socket;          // Value (socket)
    struct Node* next;      // Pointer to the next element in the list
} Node;

// Structure for managing the linked list
typedef struct {
    Node* head;             // Pointer to the first element in the list
    size_t size;            // Current number of elements in the list
    size_t maxSize;         // Maximum size based on the publisher's maxSize
} LinkedList;

// Function to initialize the list
void initList(LinkedList* list);

// Function to add key-value pairs to the list
void add(LinkedList* list, int key, SOCKET socket);

// Function to check if the list contains a specific key
int contains(LinkedList* list, int key);

// Function to get the socket based on the key
SOCKET get(LinkedList* list, int key);

// Function to remove key-value pairs from the list
void removeElement(LinkedList* list, int key);

// Function to free the resources used by the list
void freeList(LinkedList* list);

#endif // LINKED_LIST_H
