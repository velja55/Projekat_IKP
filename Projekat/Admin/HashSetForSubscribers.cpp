#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "LinkedList.h"

#include "HashSetForSubscribers.h"

// Function to initialize the HashSet
void initHashSet(HashSet* hashSet) {
    hashSet->capacity = INITIAL_HASH_TABLE_SIZE;
    hashSet->size = 0;
    hashSet->buckets = (HashNode**)calloc(hashSet->capacity, sizeof(HashNode*));
    if (hashSet->buckets == NULL) {
        printf("Error: Memory allocation failed.\n");
        exit(EXIT_FAILURE);
    }
}

// Hash function to map a key to a bucket
unsigned int hashFunction(int key, size_t capacity) {
    // FNV-1a Hashing Algorithm (a simple but effective string hashing method)
    unsigned int hash = 2166136261u;  // FNV-1a initial prime number
    hash ^= key & 0xFF;  // XOR the lower byte of the key
    hash *= 16777619u;   // Multiply by a constant prime number
    return hash % capacity;  // Return the hash value within the table capacity
}

// Function to find a publisher's node in the HashSet
HashNode* findPublisherNode(HashSet* hashSet, int publisherKey) {
    unsigned int hashIndex = hashFunction(publisherKey, hashSet->capacity);
    HashNode* current = hashSet->buckets[hashIndex];
    while (current != NULL) {
        if (current->key == publisherKey) {
            return current;  // Publisher found
        }
        current = current->next;
    }
    return NULL;  // Publisher not found
}

// Function to change the maximum size of the subscribers list for a publisher, SAmo se moze povecati velicina liste ne sme se smanjivati !!!
void changeMaxSizeofSubscribers(HashSet* hashSet, int publisherKey, size_t newMaxSize) {
    // Find the publisher node in the hash set
    HashNode* publisherNode = findPublisherNode(hashSet, publisherKey);

    // If the publisher is found, update the maxSize of its subscriber list
    if (publisherNode != NULL) {
        publisherNode->subscribers.maxSize = newMaxSize;
        printf("Max size for publisher %d changed to %zu\n", publisherKey, newMaxSize);
    }
    else {
        printf("Publisher with key %d not found.\n", publisherKey);
    }
}






// Function to resize the HashSet    -> kada se hashset popuni onda cemo raditi resizovanje
void resizeHashSet(HashSet* hashSet) {
    size_t newCapacity = hashSet->capacity * 2;
    HashNode** newBuckets = (HashNode**)calloc(newCapacity, sizeof(HashNode*));
    if (newBuckets == NULL) {
        printf("Error: Memory allocation failed during resizing.\n");
        return;
    }

    // Rehash all nodes into the new table
    for (size_t i = 0; i < hashSet->capacity; i++) {
        HashNode* current = hashSet->buckets[i];
        while (current != NULL) {
            HashNode* temp = current;
            current = current->next;

            unsigned int newHashIndex = hashFunction(temp->key, newCapacity);
            temp->next = newBuckets[newHashIndex];
            newBuckets[newHashIndex] = temp;
        }
    }

    // Free old buckets and update the hash table
    free(hashSet->buckets);
    hashSet->buckets = newBuckets;
    hashSet->capacity = newCapacity;

    printf("HashSet resized to new capacity: %zu\n", newCapacity);
}

// Function to add a subscriber to a publisher's list
void addSubscriber(HashSet* hashSet, int publisherKey, int subscriberKey, SOCKET subscriberSocket) {
    // Resize if load factor exceeds the threshold
    if ((float)(hashSet->size + 1) / hashSet->capacity > LOAD_FACTOR_THRESHOLD) {
        resizeHashSet(hashSet);
    }

    unsigned int hashIndex = hashFunction(publisherKey, hashSet->capacity);

    // Find publisher's node in the hash table
    HashNode* publisherNode = findPublisherNode(hashSet, publisherKey);
    if (publisherNode == NULL) {
        // If the publisher doesn't exist, create a new node
        publisherNode = (HashNode*)malloc(sizeof(HashNode));
        if (publisherNode == NULL) {
            printf("Error: Memory allocation failed.\n");
            return;
        }
        publisherNode->key = publisherKey;
        initList(&publisherNode->subscribers);
        publisherNode->next = hashSet->buckets[hashIndex];
        hashSet->buckets[hashIndex] = publisherNode;  // Insert at the head of the bucket
        hashSet->size++;
    }

    // Add the subscriber to the publisher's list
    add(&publisherNode->subscribers, subscriberKey, subscriberSocket);
}

// Function to remove a subscriber from a publisher's list
void removeSubscriber(HashSet* hashSet, int publisherKey, int subscriberKey) {
    HashNode* publisherNode = findPublisherNode(hashSet, publisherKey);
    if (publisherNode != NULL) {
        removeElement(&publisherNode->subscribers, subscriberKey);
    }
}

// Function to get the subscriber list for a publisher
LinkedList* getSubscribers(HashSet* hashSet, int publisherKey) {
    HashNode* publisherNode = findPublisherNode(hashSet, publisherKey);
    if (publisherNode != NULL) {
        return &publisherNode->subscribers;
    }
    return NULL;  // Publisher not found
}

// Function to free the entire HashSet
void freeHashSet(HashSet* hashSet) {
    for (size_t i = 0; i < hashSet->capacity; i++) {
        HashNode* current = hashSet->buckets[i];
        while (current != NULL) {
            HashNode* temp = current;
            current = current->next;
            freeList(&temp->subscribers);  // Free the subscribers list
            free(temp);
        }
    }
    free(hashSet->buckets);
    hashSet->buckets = NULL;
    hashSet->capacity = 0;
    hashSet->size = 0;
}
