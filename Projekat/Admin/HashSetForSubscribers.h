#ifndef HASHSET_H
#define HASHSET_H

#include <stddef.h>
#include <winsock2.h>
#include "LinkedList.h"

#define INITIAL_HASH_TABLE_SIZE 16
#define LOAD_FACTOR_THRESHOLD 0.75

typedef struct HashNode {
    int key;
    LinkedList subscribers;
    struct HashNode* next;
} HashNode;

typedef struct HashSet {
    size_t capacity;
    size_t size;
    HashNode** buckets;
    CRITICAL_SECTION criticalSection;
} HashSet;

void initHashSet(HashSet* hashSet);
unsigned int hashFunction(int key, size_t capacity);
HashNode* findPublisherNode(HashSet* hashSet, int publisherKey);
void changeMaxSizeofSubscribers(HashSet* hashSet, int publisherKey, size_t newMaxSize);
void resizeHashSet(HashSet* hashSet);
void addPublisher(HashSet* hashSet, int publisherID, size_t maxSize);
void addSubscriber(HashSet* hashSet, int publisherKey, int subscriberKey, SOCKET subscriberSocket, struct sockaddr_in addr);
void removeSubscriber(HashSet* hashSet, int publisherKey, int subscriberKey);
LinkedList* getSubscribers(HashSet* hashSet, int publisherKey);
void freeHashSet(HashSet* hashSet);
void printHashSet(HashSet* hashSet);
int* getAllPublisherIDs(HashSet* hashSet);

#endif
