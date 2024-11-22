#pragma once
#ifndef HASH_SET_H
#define HASH_SET_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include "LinkedList.h"

#define INITIAL_HASH_TABLE_SIZE 100  // Initial size of the hash table
#define LOAD_FACTOR_THRESHOLD 0.75  // Maximum load factor before resizing

// Publisher-subscriber relationship structure using hash table
typedef struct HashNode {
    int key;                    // Key for the publisher
    LinkedList subscribers;     // Linked list of subscribers

    struct HashNode* next;      // For collision handling (separate chaining)   Ako dobijemo isti buckets[hash(key)] onda ih ulancavamo na istom indexu vise HashNodo-va
} HashNode;

// Hash table structure
typedef struct {
    HashNode** buckets;         // Array of pointers to HashNode                buckets[hashfunction(key)], index nam je 
    size_t capacity;            // Current size of the hash table
    size_t size;                // Current number of elements in the hash table
} HashSet;

// Function to initialize the HashSet
void initHashSet(HashSet* hashSet);

// Hash function to map a key to a bucket
unsigned int hashFunction(int key, size_t capacity);

// Function to find a publisher's node in the HashSet
HashNode* findPublisherNode(HashSet* hashSet, int publisherKey);

// Function to change the maximum size of the subscribers list for a publisher
void changeMaxSizeofSubscribers(HashSet* hashSet, int publisherKey, size_t newMaxSize);

// Function to resize the HashSet (rehashed)
void resizeHashSet(HashSet* hashSet);

//Functio nto add Publisher
void addPublisher(HashSet* hashSet, int publisherID, size_t maxSize);

// Function to add a subscriber to a publisher's list
void addSubscriber(HashSet* hashSet, int publisherKey, int subscriberKey, SOCKET subscriberSocket);

// Function to remove a subscriber from a publisher's list
void removeSubscriber(HashSet* hashSet, int publisherKey, int subscriberKey);

// Function to get the subscriber list for a publisher
LinkedList* getSubscribers(HashSet* hashSet, int publisherKey);


void printHashSet(HashSet* hashSet);

// Function to free the entire HashSet
void freeHashSet(HashSet* hashSet);

#endif // HASH_SET_H
