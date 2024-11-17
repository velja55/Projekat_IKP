#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#include "HashSetSS.cpp"
#include <xutility>
typedef struct {
    int key;          // Ključ (ID)
    HashSet value;    // Vrednost (HashSet sa SOCKET-ima)
    int isOccupied;   // Indikator da li je ova lokacija zauzeta
} HashSetNode;

// Struktura HashSet-a koji sadrži HashSet-ove
typedef struct {
    HashSetNode* table;  // Tabela koja sadrži HashSetNode strukture
    size_t size;         // Trenutni broj elemenata
    size_t capacity;     // Trenutna veličina tabele
} HashSetOfHashSets;

// Funkcija za inicijalizaciju HashSet-a sa HashSet-ovima
void initHashSetOfHashSets(HashSetOfHashSets* set, size_t initialCapacity) {
    set->capacity = initialCapacity;
    set->size = 0;
    set->table = (HashSetNode*)calloc(set->capacity, sizeof(HashSetNode));
    if (set->table == NULL) {
        printf("Greška pri alokaciji memorije za tabelu.\n");
        exit(1);
    }
}

// Funkcija za pronalaženje indeksa pomoću hash-a i linearne probe
int findIndex(HashSetOfHashSets* set, int key) {
    unsigned int index = hash(key, set->capacity);
    unsigned int originalIndex = index;

    while (set->table[index].isOccupied && set->table[index].key != key) {
        index = (index + 1) % set->capacity;
        if (index == originalIndex) {
            return -1; // Tabela je puna
        }
    }
    return index;
}

// Dodavanje HashSet-a unutar HashSetOfHashSets
void addToHashSetOfHashSets(HashSetOfHashSets* set, int key) {
    int index = findIndex(set, key);
    if (index == -1) {
        printf("HashSet je pun.\n");
        return;
    }

    if (!set->table[index].isOccupied) {
        set->table[index].key = key;
        set->table[index].isOccupied = 1;
        initHashSet(&set->table[index].value, INITIAL_TABLE_SIZE); // Kreiraj unutrašnji HashSet
        set->size++;
    }
}

// Dohvatanje HashSet-a iz HashSetOfHashSets na osnovu ključa
HashSet* getFromHashSetOfHashSets(HashSetOfHashSets* set, int key) {
    int index = findIndex(set, key);
    if (index == -1 || !set->table[index].isOccupied) {
        return NULL; // Ključ nije pronađen
    }
    return &set->table[index].value;
}

// Oslobađanje resursa HashSet-a sa HashSet-ovima
void freeHashSetOfHashSets(HashSetOfHashSets* set) {
    for (size_t i = 0; i < set->capacity; i++) {
        if (set->table[i].isOccupied) {
            freeHashSet(&set->table[i].value); // Oslobodi unutrašnji HashSet
        }
    }
    free(set->table);
}