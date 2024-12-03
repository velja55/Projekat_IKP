#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "LinkedList.h"

// Funkcija za inicijalizaciju liste
void initList(LinkedList* list) {
    list->head = NULL;
    list->size = 0;
}

// Funkcija za dodavanje parova ključ-vrednost u listu (ključ, socket, adresa)
void add(LinkedList* list, int key, SOCKET socket, struct sockaddr_in addr) {
    if (list->size == list->maxSize) {
        printf("Max Size liste je dostignut. Nije moguće dodati pretplatnika za ovog izdavača.\n");
        return;
    }

    Node* newNode = (Node*)malloc(sizeof(Node));
    if (newNode == NULL) {
        printf("Greška pri alokaciji memorije.\n");
        return;
    }

    newNode->key = key;
    newNode->socket = socket;
    newNode->addr = addr;  // Čuvanje adrese pretplatnika
    newNode->next = list->head;  // Dodavanje na početak liste
    list->head = newNode;        // Postavljanje novog čvora kao glave liste
    list->size++;

    printf("Pretplatnik sa ID %d uspešno dodat.\n", key);
}

// Funkcija za proveru da li lista sadrži određeni ključ
int contains(LinkedList* list, int key) {
    Node* current = list->head;
    while (current != NULL) {
        if (current->key == key) {
            return 1;  // Ključ je pronađen
        }
        current = current->next;
    }
    return 0;  // Ključ nije pronađen
}

// Funkcija za dobijanje socket-a na osnovu ključa
SOCKET get(LinkedList* list, int key) {
    Node* current = list->head;
    while (current != NULL) {
        if (current->key == key) {
            return current->socket;  // Vraća socket za pronađeni ključ
        }
        current = current->next;
    }
    return INVALID_SOCKET;  // Ako ključ nije pronađen
}

// Funkcija za uklanjanje parova ključ-vrednost iz liste
void removeElement(LinkedList* list, int key) {
    Node* current = list->head;
    Node* previous = NULL;

    while (current != NULL) {
        if (current->key == key) {
            if (previous == NULL) {
                list->head = current->next;  // Uklanja prvi element u listi
            }
            else {
                previous->next = current->next;  // Preskače element
            }
            //closesocket(current->socket);  // Zatvaranje socket-a
            free(current);  // Oslobađanje memorije čvora
            list->size--;
            printf("Pretplatnik sa ID %d je uklonjen.\n", key);
            return;
        }
        previous = current;
        current = current->next;
    }
    printf("Pretplatnik sa ID %d nije pronađen.\n", key);
}

// Funkcija za oslobađanje resursa liste
void freeList(LinkedList* list) {
    Node* current = list->head;
    while (current != NULL) {
        Node* temp = current;
        current = current->next;
        closesocket(temp->socket);  // Zatvori svaki socket
        free(temp);  // Oslobađanje memorije čvora
    }
    list->head = NULL;
    list->size = 0;
    printf("Lista je uspešno oslobođena.\n");
}
