#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>

#define INITIAL_TABLE_SIZE 10  // Početna veličina tabele

// Struktura za čuvanje parova ključ-vrednost (int kao ključ, SOCKET kao vrednost)
typedef struct Node {
    int key;                // Ključ (ID)
    SOCKET socket;          // Vrednost (socket)
    struct Node* next;      // Pokazivač na sledeći element u lancu
} Node;

// Struktura HashSet-a
typedef struct {
    Node** table;           // Tabela (niz) pokazivača na liste
    size_t size;            // Trenutni broj elemenata
    size_t capacity;        // Trenutna veličina tabele
} HashSet;

// Heš funkcija za integere (ključeve)
unsigned int hash(int key, size_t capacity) {
    return key % capacity;
}

// Funkcija za kreiranje HashSet-a
void initHashSet(HashSet* set, size_t initialCapacity) {
    set->capacity = initialCapacity;
    set->size = 0;
    set->table = (Node**)malloc(sizeof(Node*) * set->capacity);
    if (set->table == NULL) {
        printf("Greška pri alokaciji memorije za tabelu.\n");
        exit(1);
    }
    for (size_t i = 0; i < set->capacity; i++) {
        set->table[i] = NULL;
    }
}

// Funkcija za proširivanje tabele kada se broj elemenata poveća
void resizeHashSet(HashSet* set) {
    size_t newCapacity = set->capacity * 2;  // Dupliranje kapaciteta
    Node** newTable = (Node**)malloc(sizeof(Node*) * newCapacity);
    if (newTable == NULL) {
        printf("Greška pri alokaciji nove memorije za tabelu.\n");
        exit(1);
    }

    // Inicijalizovanje nove tabele sa NULL
    for (size_t i = 0; i < newCapacity; i++) {
        newTable[i] = NULL;
    }

    // Premještanje svih elemenata u novu tabelu
    for (size_t i = 0; i < set->capacity; i++) {
        Node* current = set->table[i];
        while (current != NULL) {
            unsigned int newIndex = hash(current->key, newCapacity);
            Node* next = current->next;
            current->next = newTable[newIndex];
            newTable[newIndex] = current;
            current = next;
        }
    }

    // Oslobađanje stare tabele i postavljanje nove
    free(set->table);
    set->table = newTable;
    set->capacity = newCapacity;
}

// Funkcija za dodavanje parova ključ-vrednost u HashSet
void add(HashSet* set, int key, SOCKET socket) {
    // Proširivanje tabele ako je potrebno
    if (set->size >= set->capacity / 2) {
        resizeHashSet(set);
    }

    unsigned int index = hash(key, set->capacity);  // Izračunavanje heš koda
    Node* newNode = (Node*)malloc(sizeof(Node));
    if (newNode == NULL) {
        printf("Greška pri alokaciji memorije.\n");
        return;
    }

    newNode->key = key;
    newNode->socket = socket;
    newNode->next = set->table[index];  // Dodavanje na početak liste (lanciranje sudara)
    set->table[index] = newNode;        // Stavljanje nove liste na odgovarajući index
    set->size++;
}

// Funkcija za proveru da li HashSet sadrži određeni ključ
int contains(HashSet* set, int key) {
    unsigned int index = hash(key, set->capacity);
    Node* current = set->table[index];
    while (current != NULL) {
        if (current->key == key) {
            return 1;  // Ključ je pronađen
        }
        current = current->next;
    }
    return 0;  // Ključ nije pronađen
}

// Funkcija za dobijanje socket-a na osnovu ključa
SOCKET get(HashSet* set, int key) {
    unsigned int index = hash(key, set->capacity);
    Node* current = set->table[index];
    while (current != NULL) {
        if (current->key == key) {
            return current->socket;  // Vraća socket za pronađeni ključ
        }
        current = current->next;
    }
    return INVALID_SOCKET;  // Ako ključ nije pronađen
}

// Funkcija za uklanjanje parova ključ-vrednost iz HashSet-a
void removeElement(HashSet* set, int key) {
    unsigned int index = hash(key, set->capacity);
    Node* current = set->table[index];
    Node* previous = NULL;

    while (current != NULL) {
        if (current->key == key) {
            if (previous == NULL) {
                set->table[index] = current->next;  // Prvi element u listi
            }
            else {
                previous->next = current->next;  // Uklanjanje iz liste
            }
            closesocket(current->socket);  // Zatvaranje socket-a
            free(current);
            set->size--;
            return;
        }
        previous = current;
        current = current->next;
    }
}

// Funkcija za oslobađanje resursa HashSet-a
void freeHashSet(HashSet* set) {
    for (size_t i = 0; i < set->capacity; i++) {
        Node* current = set->table[i];
        while (current != NULL) {
            Node* temp = current;
            current = current->next;
            closesocket(temp->socket);  // Zatvori svaki socket
            free(temp);
        }
    }
    free(set->table);
}