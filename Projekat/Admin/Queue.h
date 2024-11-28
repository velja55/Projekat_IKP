#ifndef QUEUE_H
#define QUEUE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Maksimalna veličina poruke
#define MAX_MESSAGE_SIZE 256

// Početni kapacitet reda
#define INITIAL_CAPACITY 4

// Struktura za čvor reda
typedef struct {
    int* idPublishers;             // Niz ID-ova publishera
    char** messages;               // Niz poruka
    int front;                     // Indeks prvog elementa u redu
    int rear;                      // Indeks poslednjeg elementa u redu
    int size;                      // Trenutna veličina reda
    int capacity;                  // Kapacitet reda
} Queue;

// Funkcija za inicijalizaciju reda
void initQueue(Queue* queue);

// Funkcija za dodavanje elemenata u red (enqueue)
void enqueue(Queue* queue, int idPublisher, const char* message);

// Funkcija za uklanjanje elemenata iz reda (dequeue)
int dequeue(Queue* queue, int* idPublisher, char* message);

// Funkcija za proveru da li je red prazan
int isQueueEmpty(Queue* queue);

// Funkcija za širenje kapaciteta reda kada se popuni
void expandQueue(Queue* queue);

// Funkcija za oslobađanje memorije reda
void freeQueue(Queue* queue);

#endif // QUEUE_H
