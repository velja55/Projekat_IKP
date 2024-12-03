#ifndef LINKEDLIST_H
#define LINKEDLIST_H

#include <winsock2.h>  // For SOCKET and sockaddr_in

// Struktura za čvor u listi
typedef struct Node {
    int key;                 // Ključ
    SOCKET socket;           // Socket za komunikaciju
    struct sockaddr_in addr; // Adresa pretplatnika
    struct Node* next;       // Sledeći čvor u listi
} Node;

// Struktura za listu
typedef struct LinkedList {
    Node* head;      // Početak liste
    int size;        // Veličina liste
    int maxSize;     // Maksimalni broj elemenata u listi
} LinkedList;

// Funkcija za inicijalizaciju liste
void initList(LinkedList* list);

// Funkcija za dodavanje parova ključ-vrednost u listu
void add(LinkedList* list, int key, SOCKET socket, struct sockaddr_in addr);

// Funkcija za proveru da li lista sadrži određeni ključ
int contains(LinkedList* list, int key);

// Funkcija za dobijanje socket-a na osnovu ključa
SOCKET get(LinkedList* list, int key);

// Funkcija za uklanjanje parova ključ-vrednost iz liste
void removeElement(LinkedList* list, int key);

// Funkcija za oslobađanje resursa liste
void freeList(LinkedList* list);

#endif // LINKEDLIST_H
