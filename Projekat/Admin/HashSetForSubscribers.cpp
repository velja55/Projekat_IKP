#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <WinSock2.h>
#include <windows.h>
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
    InitializeCriticalSection(&hashSet->criticalSection); // Initialize Critical Section
}

// Hash function to map a key to a bucket
unsigned int hashFunction(int key, size_t capacity) {
    unsigned int hash = 2166136261u;
    hash ^= key & 0xFF;
    hash *= 16777619u;
    return hash % capacity;
}

// Function to find a publisher's node in the HashSet
HashNode* findPublisherNode(HashSet* hashSet, int publisherKey) {
    unsigned int hashIndex = hashFunction(publisherKey, hashSet->capacity);
    HashNode* current = hashSet->buckets[hashIndex];
    while (current != NULL) {                                                           //zbog ulancavanja na istom indeexu moramo proci kroz ulancane sve da proverimo da li to taj id
        if (current->key == publisherKey) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

// Function to change the maximum size of the subscribers list for a publisher
void changeMaxSizeofSubscribers(HashSet* hashSet, int publisherKey, size_t newMaxSize) {
    EnterCriticalSection(&hashSet->criticalSection);
    HashNode* publisherNode = findPublisherNode(hashSet, publisherKey);

    if (publisherNode != NULL) {
        publisherNode->subscribers.maxSize = newMaxSize;
        printf("Max size for publisher %d changed to %zu\n", publisherKey, newMaxSize);
    }
    else {
        printf("Publisher with key %d not found.\n", publisherKey);
    }
    LeaveCriticalSection(&hashSet->criticalSection);
}

// Function to resize the HashSet
void resizeHashSet(HashSet* hashSet) {
    EnterCriticalSection(&hashSet->criticalSection);

    size_t newCapacity = hashSet->capacity * 2;
    HashNode** newBuckets = (HashNode**)calloc(newCapacity, sizeof(HashNode*));
    if (newBuckets == NULL) {
        printf("Error: Memory allocation failed during resizing.\n");
        LeaveCriticalSection(&hashSet->criticalSection);
        return;
    }

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

    free(hashSet->buckets);
    hashSet->buckets = newBuckets;
    hashSet->capacity = newCapacity;

    printf("HashSet resized to new capacity: %zu\n", newCapacity);
    LeaveCriticalSection(&hashSet->criticalSection);
}

// Function to add a publisher to the HashSet
void addPublisher(HashSet* hashSet, int publisherID, size_t maxSize) {
    EnterCriticalSection(&hashSet->criticalSection);

    if ((float)(hashSet->size + 1) / hashSet->capacity > LOAD_FACTOR_THRESHOLD) {
        resizeHashSet(hashSet);
    }

    unsigned int hashIndex = hashFunction(publisherID, hashSet->capacity);
    if (findPublisherNode(hashSet, publisherID) != NULL) {
        printf("Publisher with ID %d already exists.\n", publisherID);
        LeaveCriticalSection(&hashSet->criticalSection);
        return;
    }

    HashNode* newNode = (HashNode*)malloc(sizeof(HashNode));
    if (newNode == NULL) {
        printf("Error: Memory allocation failed.\n");
        LeaveCriticalSection(&hashSet->criticalSection);
        return;
    }

    newNode->key = publisherID;
    initList(&newNode->subscribers);
    newNode->subscribers.maxSize = maxSize;
    newNode->next = hashSet->buckets[hashIndex];
    hashSet->buckets[hashIndex] = newNode;
    hashSet->size++;

    printf("Publisher with ID %d added successfully.\n", publisherID);
    LeaveCriticalSection(&hashSet->criticalSection);
}

// Function to add a subscriber to a publisher's list
bool addSubscriber(HashSet* hashSet, int publisherKey, int subscriberKey, SOCKET subscriberSocket, struct sockaddr_in addr) {
    EnterCriticalSection(&hashSet->criticalSection);

    HashNode* publisherNode = findPublisherNode(hashSet, publisherKey);
    if (publisherNode == NULL) {
        printf("Error: Publisher with key %d not found. Cannot add subscriber %d.\n", publisherKey, subscriberKey);
        LeaveCriticalSection(&hashSet->criticalSection);
        return false;
    }

    //proveravmo da li dodajemo istog subscribra na istog publsihera

    int odgovor = contains(&publisherNode->subscribers, subscriberKey);
    if (odgovor == 1)
    {
        printf("Can't Add Subsriber to same Publisher 2 times \n");
        LeaveCriticalSection(&hashSet->criticalSection);
        return false;
    }


    if (add(&publisherNode->subscribers, subscriberKey, subscriberSocket, addr) == true) {
        LeaveCriticalSection(&hashSet->criticalSection);
        return true;

    }
    else {
        LeaveCriticalSection(&hashSet->criticalSection);
        return false;
    }

   // printf("Subscriber %d added to publisher %d successfully.\n", subscriberKey, publisherKey);   vec ispisujemo u listi da je uspesno dodat

}


// Function to remove a subscriber from a publisher's list
bool removeSubscriber(HashSet* hashSet, int publisherKey, int subscriberKey) {
    EnterCriticalSection(&hashSet->criticalSection);

    HashNode* publisherNode = findPublisherNode(hashSet, publisherKey);
    if (publisherNode != NULL) {
        if (removeElement(&publisherNode->subscribers, subscriberKey) == true) {
            LeaveCriticalSection(&hashSet->criticalSection);
            //ne mora ovde printf vec ga imamo u listi
            return true;
        }
        else {
            LeaveCriticalSection(&hashSet->criticalSection);
            return false;
        }
    }
    else {
        //ne postoji publisher

        printf("Publisher with that ID doesnt exist \n");
        LeaveCriticalSection(&hashSet->criticalSection);
        return false;

    }

}

HashNode* findPublisherNodeBySubscriberID(HashSet* hashSet, int subscriberID) {
    EnterCriticalSection(&hashSet->criticalSection);
    // Iterate through all buckets in the hash set
    for (size_t i = 0; i < hashSet->capacity; ++i) {
        HashNode* current = hashSet->buckets[i];

        // Iterate through all publishers in the current bucket
        while (current != NULL) {
            // Check if the subscriber exists in the current publisher's subscribers list
            if (contains(&current->subscribers, subscriberID) ==1) {
                LeaveCriticalSection(&hashSet->criticalSection);
                return current; // Return the HashNode where the subscriber is found
            }
            current = current->next; // Move to the next HashNode in the list
        }
    }

    LeaveCriticalSection(&hashSet->criticalSection);
    return NULL; // Return NULL if the subscriber with the given ID was not found
}


// Function to get the subscriber list for a publisher
LinkedList* getSubscribers(HashSet* hashSet, int publisherKey) {
    EnterCriticalSection(&hashSet->criticalSection);

    HashNode* publisherNode = findPublisherNode(hashSet, publisherKey);
    if (publisherNode != NULL) {
        LeaveCriticalSection(&hashSet->criticalSection);
        return &publisherNode->subscribers;
    }

    LeaveCriticalSection(&hashSet->criticalSection);
    return NULL;
}

 

// Function to free the entire HashSet
void freeHashSet(HashSet* hashSet) {
    EnterCriticalSection(&hashSet->criticalSection);

    for (size_t i = 0; i < hashSet->capacity; i++) {
        HashNode* current = hashSet->buckets[i];
        while (current != NULL) {
            HashNode* temp = current;
            current = current->next;
            freeList(&temp->subscribers);
            free(temp);
        }
    }
    free(hashSet->buckets);
    DeleteCriticalSection(&hashSet->criticalSection);
}

// Function to print the HashSet
void printHashSet(HashSet* hashSet) {
    EnterCriticalSection(&hashSet->criticalSection);

    for (size_t i = 0; i < hashSet->capacity; i++) {
        HashNode* current = hashSet->buckets[i];
        if (current != NULL) {
            printf("Bucket %d:\n", i);
            while (current != NULL) {
                printf("  Publisher ID: %d\n", current->key);
                printf("    Max subscribers: %zu\n", current->subscribers.maxSize);
                Node* subscriber = current->subscribers.head;
                while (subscriber != NULL) {
                    printf("      Subscriber ID: %d\n", subscriber->key);
                    subscriber = subscriber->next;
                }
                current = current->next;
            }
        }
    }

    LeaveCriticalSection(&hashSet->criticalSection);
}


int* getAllPublisherIDs(HashSet* hashSet) {
    EnterCriticalSection(&hashSet->criticalSection);

    int* publisherIDs = (int*)malloc(hashSet->size * sizeof(int));
    if (!publisherIDs) {
        LeaveCriticalSection(&hashSet->criticalSection);
        return NULL;
    }

    size_t index = 0;
    for (size_t i = 0; i < hashSet->capacity; i++) {
        HashNode* current = hashSet->buckets[i];
        while (current) {
            publisherIDs[index++] = current->key; // Add publisher ID to array
            current = current->next;
        }
    }

    LeaveCriticalSection(&hashSet->criticalSection);
    return publisherIDs;
}
