#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>  // For inet_ntop

#include "HashSetForSubscribers.h"
#include "Queue.h" 

#define MAX_BUFFER_SIZE 1024
#define PUBLISHER_PORT 12345

#define SUBSCRIBER_PORT 12346

HashSet* glavniHashSet;
CRITICAL_SECTION criticalSection;
Queue* queueZaPoruke;
HANDLE hSemaphore;




// Struktura za prosleđivanje podataka nitima
typedef struct {
    SOCKET sockfd;
    struct sockaddr_in client_addr;
    int addr_len;
} client_data_t;



//DWORD WINAPI WorkerFunction(LPVOID lpParam) {
//    while (1) {
//        // Wait for the semaphore to be released (signal from the main thread)
//        WaitForSingleObject(hSemaphore, INFINITE);
//
//        // Process the message from the queue (ensure thread-safe access)
//        int publisherID;
//        char* message = (char*)malloc(256 * sizeof(char));
//        if (dequeue(queueZaPoruke, &publisherID, message) == 0) {
//            // Assuming sendToSubscribers sends the message to all subscribers
//           // sendToSubscribers(publisherID, message);  // Implement this function to send to subscribers
//            printf("Worker: Processing message from Publisher %d: %s\n", publisherID, message);
//        }
//        free(message);  // Free memory allocated for the message
//    }
//    return 0;
//}

// Funkcija za parsiranje poruke
// Function to parse the message and extract ID, naziv, and maxsize
void parse_message(char* buffer, int* operacija, int* id, size_t* maxSize, char* poruka, struct sockaddr_in* client_addr) {
    char id_str[16], maxsize_str[16], operacija_str[16];
    char poruka_str[256];  // Bafer za poruku sada ima dovoljno prostora
    char* context;  // Context for strtok_s

    // Prvi token je operacija (da li je prijava ili normalna poruka)
    char* token = strtok_s(buffer, "|", &context);

    if (token != NULL) {
        sscanf_s(token, "operacija=%15s", operacija_str, (unsigned)_countof(operacija_str));
        *operacija = atoi(operacija_str);
    }

    // Drugi token je ID (port)
    token = strtok_s(NULL, "|", &context);

    if (token != NULL) {
        sscanf_s(token, "id=%15s", id_str, (unsigned)_countof(id_str));
        *id = ntohs(client_addr->sin_port);  // Pretpostavljam da ovde koristiš port klijenta
    }

    // Treći token je maxsize ili poruka
    token = strtok_s(NULL, "|", &context);
    if (*operacija == 1) {
        if (token != NULL) {
            sscanf_s(token, "maxsize=%15s", maxsize_str, (unsigned)_countof(maxsize_str));
            *maxSize = (size_t)atoi(maxsize_str); // Pretvaranje u size_t
        }
        poruka[0] = '\0';  // Nema poruke u ovom slučaju
    }
    else {
        if (token != NULL) {
            sscanf_s(token, "message=%255s", poruka_str, (unsigned)_countof(poruka_str));  // Čitanje poruke
            strcpy_s(poruka, 256, poruka_str);  // Kopiranje poruke
            *maxSize = 0;  // Postavljamo maxSize na 0 jer nije relevantno za operaciju 2
        }
    }

    // Ispis primljenih detalja
    printf("Received message details:\n");
    printf("Operacija: %d\n", *operacija);
    printf("ID (Port): %d\n", *id);
    if (*operacija == 1) {
        printf("MaxSize: %zu\n", *maxSize);
    }
    else {
        printf("Message: %s\n", poruka);
    }

    // Ispis informacija o izvoru poruke (IP adresa i port)
    char client_ip[INET_ADDRSTRLEN];
    if (inet_ntop(AF_INET, &client_addr->sin_addr, client_ip, sizeof(client_ip)) == NULL) {
        printf("inet_ntop failed\n");
    }
    else {
        printf("Message received from IP: %s, Port: %d\n", client_ip, ntohs(client_addr->sin_port));
    }
}


DWORD WINAPI publisher_processing_thread(LPVOID arg) {
    SOCKET sockfd = *(SOCKET*)arg;
    struct sockaddr_in client_addr;
    int addr_len = sizeof(client_addr);
    char buffer[MAX_BUFFER_SIZE];
    int bytes_received;

    printf("Thread for processing client messages is running...\n");

    while (1) {
        // Receive message
        bytes_received = recvfrom(sockfd, buffer, MAX_BUFFER_SIZE, 0, (struct sockaddr*)&client_addr, &addr_len);
        if (bytes_received == SOCKET_ERROR) {
            printf("recvfrom failed\n");
            continue;
        }

        // Null-terminate the string
        buffer[bytes_received] = '\0';

        // Parse the message
        int flag;
        int publisherID;
        size_t maxSize;
        char* poruka = (char*)malloc(256 * sizeof(char));
        parse_message(buffer,&flag, &publisherID, &maxSize,poruka, &client_addr);

        if (flag == 1) {
            addPublisher(glavniHashSet, publisherID, maxSize);              //critical section je u funckiji
            printHashSet(glavniHashSet);
        }
        else {
            enqueue(queueZaPoruke, publisherID, poruka);
            printQueue(queueZaPoruke);
        }
        // Send acknowledgment back to the client
        sendto(sockfd, "Acknowledged", strlen("Acknowledged"), 0, (struct sockaddr*)&client_addr, addr_len);

        // Print the current state of the HashSet
        printHashSet(glavniHashSet);
    }

    return 0;
}

/*
//do ovde sve dobro uradi
void sendPublisherIDsToSubscriber(SOCKET subscriberSocket, int* publisherIDs, size_t count, struct sockaddr_in* clientAddr) {
    if (!publisherIDs || count == 0) {
        const char* errorMsg = "No publishers available";
        sendto(subscriberSocket, errorMsg, strlen(errorMsg), 0, (struct sockaddr*)clientAddr, sizeof(*clientAddr));
        return;
    }

    // Send the IDs as a single data packet
    int bytesSent = sendto(subscriberSocket, (char*)publisherIDs, count * sizeof(int), 0, (struct sockaddr*)clientAddr, sizeof(*clientAddr));  //ne posalje dobre vrednosti
    if (bytesSent == SOCKET_ERROR) {
        printf("Failed to send publisher IDs to subscriber\n");
    }
    else {
        printf("Sent %zu publisher IDs to subscriber\n", count);
    }
}
*/

void sendPublisherIDsToSubscriber(SOCKET subscriberSocket, int* publisherIDs, size_t count, struct sockaddr_in* clientAddr) {
    if (!publisherIDs || count == 0) {
        const char* errorMsg = "No publishers available";
        sendto(subscriberSocket, errorMsg, strlen(errorMsg), 0, (struct sockaddr*)clientAddr, sizeof(*clientAddr));
        return;
    }

    // Konvertovanje niza ID-ova u string (CSV format)
    char buffer[MAX_BUFFER_SIZE];
    int offset = 0;

    for (size_t i = 0; i < count; i++) {
        offset += snprintf(buffer + offset, MAX_BUFFER_SIZE - offset, "%d,", publisherIDs[i]);
        if (offset >= MAX_BUFFER_SIZE) {
            printf("Error: Buffer overflow while serializing publisher IDs\n");
            return;
        }
    }

    // Ukloni poslednji zarez
    if (count > 0) {
        buffer[offset - 1] = '\0';
    }

    // Pošalji serijalizovane podatke klijentu
    int bytesSent = sendto(subscriberSocket, buffer, strlen(buffer), 0, (struct sockaddr*)clientAddr, sizeof(*clientAddr));
    if (bytesSent == SOCKET_ERROR) {
        printf("Failed to send publisher IDs to subscriber\n");
    }
    else {
        printf("Sent %zu publisher IDs to subscriber: %s\n", count, buffer);
    }
}


DWORD WINAPI subscriber_processing_thread(LPVOID arg) {
    SOCKET subscriberSocket = *(SOCKET*)arg;
    struct sockaddr_in clientAddr;
    int addrLen = sizeof(clientAddr);
    char buffer[MAX_BUFFER_SIZE];
    int bytesReceived;
    printf("USO \n");
    while (1) {
        // Receive subscription requests
        bytesReceived = recvfrom(subscriberSocket, buffer, MAX_BUFFER_SIZE, 0, (struct sockaddr*)&clientAddr, &addrLen);
        if (bytesReceived == SOCKET_ERROR) {
            printf("recvfrom failed for subscriber\n");
            continue;
        }

        buffer[bytesReceived] = '\0'; // Null-terminate received data

        printf("Message received from Subscriber %s \n", buffer);

        if (strncmp(buffer, "get_publishers", 14) == 0) {
            // Gather publisher IDs
            int* publisherIDs = getAllPublisherIDs(glavniHashSet);

            if (publisherIDs) {
                sendPublisherIDsToSubscriber(subscriberSocket, publisherIDs, glavniHashSet->size, &clientAddr);
                free(publisherIDs); // Free allocated memory
            }
            else {
                printf("Failed to allocate memory for publisher IDs\n");
            }
        }
        else if (strncmp(buffer, "subscribe:", 10) == 0) {
            int publisherID = atoi(buffer + 10);
            int subscriberID = ntohs(clientAddr.sin_port); // Use client port as subscriber ID

            // Add subscriber to the publisher
            addSubscriber(glavniHashSet, publisherID, subscriberID, subscriberSocket);

            // Notify the publisher about the new subscriber
            struct sockaddr_in publisherAddr;
            memset(&publisherAddr, 0, sizeof(publisherAddr));
            publisherAddr.sin_family = AF_INET;
                publisherAddr.sin_addr.s_addr = INADDR_ANY;  // Assuming the publisher is running locally
            publisherAddr.sin_port = htons(publisherID);             // Publisher's port is its ID

            char notificationMessage[MAX_BUFFER_SIZE];
            snprintf(notificationMessage, MAX_BUFFER_SIZE, "New subscriber:%d", subscriberID);

            int bytesSent = sendto(subscriberSocket, notificationMessage, strlen(notificationMessage), 0,
                (struct sockaddr*)&publisherAddr, sizeof(publisherAddr));

            // Acknowledge subscription
            sendto(subscriberSocket, "Subscribed", strlen("Subscribed"), 0, (struct sockaddr*)&clientAddr, addrLen);
            printf("Subscriber %d added to Publisher %d\n", subscriberID, publisherID);

            printHashSet(glavniHashSet);

        }
        else {
            printf("Unknown subscriber request: %s\n", buffer);
        }
    }

    return 0;
}



int main() {
    WSADATA wsaData;
    SOCKET publisherSocket;
    SOCKET subscriberSocket;
    struct sockaddr_in publisherAddr, subscriberAddr;


    InitializeCriticalSection(&criticalSection);

    // Initialize the HashSet
    glavniHashSet = (HashSet*)malloc(sizeof(HashSet));
    queueZaPoruke = (Queue*)malloc(sizeof(Queue));
    if (glavniHashSet == NULL) {
        printf("Error: Memory allocation failed for glavniHashSet.\n");
        return EXIT_FAILURE;
    }
    initHashSet(glavniHashSet);
    initQueue(queueZaPoruke);

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup failed\n");
        return 1;
    }

    // Create a UDP socket
    publisherSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (publisherSocket == INVALID_SOCKET) {
        printf("Publisher Socket creation failed\n");
        WSACleanup();
        return 1;
    }

    subscriberSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (subscriberSocket == INVALID_SOCKET) {
        printf("Subrciber Socket creation failed\n");
        WSACleanup();
        return 1;
    }

    // Set up server configuration
    memset(&publisherAddr, 0, sizeof(publisherAddr));
    publisherAddr.sin_family = AF_INET;
    publisherAddr.sin_addr.s_addr = INADDR_ANY;
    publisherAddr.sin_port = htons(PUBLISHER_PORT);

    // Bind the socket to the address
    if (bind(publisherSocket, (struct sockaddr*)&publisherAddr, sizeof(publisherAddr)) == SOCKET_ERROR) {
        printf("Bind failed\n");
        closesocket(publisherSocket);
        WSACleanup();
        return 1;
    }


    printf("Server is running on port %d...\n", PUBLISHER_PORT);

    // Bind subscriber socket
    subscriberAddr.sin_family = AF_INET;
    subscriberAddr.sin_addr.s_addr = INADDR_ANY;
    subscriberAddr.sin_port = htons(SUBSCRIBER_PORT);

    if (bind(subscriberSocket, (struct sockaddr*)&subscriberAddr, sizeof(subscriberAddr)) == SOCKET_ERROR) {
        printf("Subscriber socket binding failed\n");
        closesocket(publisherSocket);
        closesocket(subscriberSocket);
        WSACleanup();
        return 1;
    }


    // Create the persistent thread for publisher processing
    HANDLE publisher_thread = CreateThread(
        NULL,
        0,
        publisher_processing_thread,
        &publisherSocket,
        0,
        NULL
    );

    if (publisher_thread == NULL) {
        printf("Failed to create processing thread\n");
        closesocket(publisherSocket);
        closesocket(subscriberSocket);
        WSACleanup();
        return 1;
    }



    // Wait for the processing thread to finish (optional)

    HANDLE subscriberThread = CreateThread(NULL, 0, subscriber_processing_thread, &subscriberSocket, 0, NULL);

    if (subscriberThread ==NULL) {
        printf("Failed to create Subscriber thread\n");
        closesocket(publisherSocket);
        closesocket(subscriberSocket);
        WSACleanup();
        return 1;
    }






    WaitForSingleObject(publisher_thread, INFINITE);
    WaitForSingleObject(subscriberThread, INFINITE);




    CloseHandle(publisher_thread);
    CloseHandle(subscriberThread);


    // Cleanup resources
    closesocket(publisherSocket);
    closesocket(subscriberSocket);
    WSACleanup();

    DeleteCriticalSection(&criticalSection);

    // Free the HashSet when done
    freeHashSet(glavniHashSet);
    freeQueue(queueZaPoruke);
    free(glavniHashSet);
    free(queueZaPoruke);
    

    return 0;
}