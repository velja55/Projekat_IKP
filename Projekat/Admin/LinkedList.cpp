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
bool add(LinkedList* list, int key, SOCKET socket, struct sockaddr_in addr) {
    if (list->size == list->maxSize) {
        printf("Max Size liste za Subscribere je dostignut. Nije moguce dodati pretplatnika za ovog izdavaca.\n");
        return false;
    }

    Node* newNode = (Node*)malloc(sizeof(Node));
    if (newNode == NULL) {
        printf("Greška pri alokaciji memorije.\n");
        return false;
    }

    newNode->key = key;
    newNode->socket = socket;
    newNode->addr = addr;  // Čuvanje adrese pretplatnika
    newNode->next = list->head;  // Dodavanje na početak liste
    list->head = newNode;        // Postavljanje novog čvora kao glave liste
    list->size++;

    printf("Pretplatnik sa ID %d uspesno dodat.\n", key);

    return true;
}

// Funkcija za proveru da li lista sadrži određeno subscribera
int contains(LinkedList* list, int key) {
    Node* current = list->head;
    while (current != NULL) {
        if (current->key == key) {
            return 1; // Ključ je pronađen
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
//funk za dobijanje adrese sub-a
sockaddr_in getAddr(LinkedList* list, int key) {
    Node* current = list->head;
    while (current != NULL) {
        if (current->key == key) {
            return current->addr;  // Vraća adresu za pronađeni 
        }
        current = current->next;
    }

    // Return a default sockaddr_in with invalid values
    sockaddr_in invalidAddr;
    memset(&invalidAddr, 0, sizeof(invalidAddr));  // Zero out the structure
    return invalidAddr;  // Return an invalid address


}



// Funkcija za uklanjanje parova ključ-vrednost iz liste
bool removeElement(LinkedList* list, int key) {
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
            return true;
        }
        previous = current;
        current = current->next;
    }
    printf("Pretplatnik sa ID %d nije pronadjen.\n", key);
    return false;
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
    printf("Lista je uspesno oslobodjena.\n");
}
