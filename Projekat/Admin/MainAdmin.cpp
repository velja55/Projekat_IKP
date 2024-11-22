#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>  // For inet_ntop

#include "HashSetForSubscribers.h"

#define MAX_BUFFER_SIZE 1024
#define PORT 12345
HashSet* glavniHashSet;
CRITICAL_SECTION criticalSection;


// Struktura za prosleđivanje podataka nitima
typedef struct {
    SOCKET sockfd;
    struct sockaddr_in client_addr;
    int addr_len;
} client_data_t;

// Funkcija za parsiranje poruke
// Function to parse the message and extract ID, naziv, and maxsize
void parse_message(char* buffer, int* id, size_t* maxSize, struct sockaddr_in* client_addr) {
    char* token;
    char id_str[16], naziv[MAX_BUFFER_SIZE], maxsize_str[16];
    char* context;  // Context for strtok_s

    // First token is ID (port)
    token = strtok_s(buffer, "|", &context);
    if (token != NULL) {
        sscanf_s(token, "id=%s", id_str, sizeof(id_str));
        *id = atoi(id_str); // Convert to integer
    }

    // Second token is naziv
    token = strtok_s(NULL, "|", &context);
    if (token != NULL) {
        sscanf_s(token, "naziv=%s", naziv, sizeof(naziv));
    }

    // Third token is maxsize
    token = strtok_s(NULL, "|", &context);
    if (token != NULL) {
        sscanf_s(token, "maxsize=%s", maxsize_str, sizeof(maxsize_str));
        *maxSize = (size_t)atoi(maxsize_str); // Convert to size_t
    }

    // Print received details
    printf("Received message details:\n");
    printf("ID (Port): %d\n", *id);
    printf("Naziv: %s\n", naziv);
    printf("MaxSize: %zu\n", *maxSize);




    // Print source information (IP address and port)
    char client_ip[INET_ADDRSTRLEN];
    if (inet_ntop(AF_INET, &client_addr->sin_addr, client_ip, sizeof(client_ip)) == NULL) {
        printf("inet_ntop failed\n");
    }
    else {
        printf("Message received from IP: %s, Port: %d\n", client_ip, ntohs(client_addr->sin_port));
    }
}



DWORD WINAPI handle_client(LPVOID arg) {
    client_data_t* data = (client_data_t*)arg;
    char buffer[MAX_BUFFER_SIZE];
    int bytes_received;

    // Receive data from the client
    bytes_received = recvfrom(data->sockfd, buffer, MAX_BUFFER_SIZE, 0, (struct sockaddr*)&data->client_addr, &data->addr_len);
    if (bytes_received == SOCKET_ERROR) {
        printf("recvfrom failed\n");
        free(data);
        return 1;
    }

    // Null-terminate the string
    buffer[bytes_received] = '\0';

    // Extract details from the message
    int publisherID;
    size_t maxSize;
    parse_message(buffer, &publisherID, &maxSize, &data->client_addr);
    // Parse and process the message

    // Lock the critical section before modifying the HashSet
    EnterCriticalSection(&criticalSection); // Lock

    // Extract information from the parsed message and update the HashSet
    int id = 0, maxsize = 0;
    char naziv[MAX_BUFFER_SIZE];
    sscanf_s(buffer, "id=%d|naziv=%s|maxsize=%zu", &id, naziv, (unsigned)_countof(naziv), &maxsize);


    printf("ID: main : %d", id);
    // Add the publisher to the HashSet
    addPublisher(glavniHashSet, id, maxsize);

    // Unlock the critical section after modifying the HashSet
    LeaveCriticalSection(&criticalSection); // Unlock

    // Send acknowledgment back to the client
    sendto(data->sockfd, "Acknowledged", strlen("Acknowledged"), 0, (struct sockaddr*)&data->client_addr, data->addr_len);


    free(data);
    return 0;
}


int main() {
    WSADATA wsaData;
    SOCKET sockfd;
    struct sockaddr_in server_addr;
    InitializeCriticalSection(&criticalSection);



    // Initialize the HashSet
     glavniHashSet = (HashSet*)malloc(sizeof(HashSet));
    if (glavniHashSet == NULL) {
        printf("Error: Memory allocation failed for glavniHashSet.\n");
        return EXIT_FAILURE;
    }
    initHashSet(glavniHashSet);

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup failed\n");
        return 1;
    }

    // Create a UDP socket
    sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sockfd == INVALID_SOCKET) {
        printf("Socket creation failed\n");
        WSACleanup();
        return 1;
    }

    // Set up server configuration
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Bind the socket to the address
    if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        printf("Bind failed\n");
        closesocket(sockfd);
        WSACleanup();
        return 1;
    }

    printf("Server is running on port %d...\n", PORT);

    // Loop to accept multiple messages
    while (1) {
        client_data_t* data = (client_data_t*)malloc(sizeof(client_data_t));
        if (data == NULL) {
            printf("Memory allocation failed\n");
            continue;
        }

        // Set up client data
        data->sockfd = sockfd;
        data->addr_len = sizeof(data->client_addr);

        // Create a thread to handle the client
        HANDLE thread_handle = CreateThread(
            NULL,
            0,
            handle_client,
            (LPVOID)data,
            0,
            NULL
        );

        if (thread_handle == NULL) {
            printf("CreateThread failed\n");
            free(data);
        }
        else {
            CloseHandle(thread_handle);
        }
    }

    printHashSet(glavniHashSet);

    // Cleanup resources
    closesocket(sockfd);
    WSACleanup();

    DeleteCriticalSection(&criticalSection);

    // Free the HashSet when done
    freeHashSet(glavniHashSet);
    free(glavniHashSet);

    return 0;
}
