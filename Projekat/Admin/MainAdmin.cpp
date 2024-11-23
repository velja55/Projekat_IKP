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
    char id_str[16], maxsize_str[16];
    char* context;  // Context for strtok_s

    // First token is ID (port)
    char* token = strtok_s(buffer, "|", &context);
    if (token != NULL) {
        sscanf_s(token, "id=%15s", id_str, (unsigned)_countof(id_str));
        *id = ntohs(client_addr->sin_port);
    }

    // Second token is naziv (not used here but parsed)
    token = strtok_s(NULL, "|", &context);

    // Third token is maxsize
    token = strtok_s(NULL, "|", &context);
    if (token != NULL) {
        sscanf_s(token, "maxsize=%15s", maxsize_str, (unsigned)_countof(maxsize_str));
        *maxSize = (size_t)atoi(maxsize_str); // Convert to size_t
    }

    // Print received details
    printf("Received message details:\n");
    printf("ID (Port): %d\n", *id);
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

    // Lock the critical section before modifying the HashSet
    EnterCriticalSection(&criticalSection);

    // Add the publisher to the HashSet
    addPublisher(glavniHashSet, publisherID, maxSize);

    // Unlock the critical section after modifying the HashSet
    LeaveCriticalSection(&criticalSection);

    // Send acknowledgment back to the client
    sendto(data->sockfd, "Acknowledged", strlen("Acknowledged"), 0, (struct sockaddr*)&data->client_addr, data->addr_len);

    free(data);
    return 0;
}

DWORD WINAPI client_processing_thread(LPVOID arg) {
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
        int publisherID;
        size_t maxSize;
        parse_message(buffer, &publisherID, &maxSize, &client_addr);

        // Lock the critical section before modifying the HashSet
        EnterCriticalSection(&criticalSection);

        // Add the publisher to the HashSet
        addPublisher(glavniHashSet, publisherID, maxSize);

        // Unlock the critical section after modifying the HashSet
        LeaveCriticalSection(&criticalSection);

        // Send acknowledgment back to the client
        sendto(sockfd, "Acknowledged", strlen("Acknowledged"), 0, (struct sockaddr*)&client_addr, addr_len);

        // Print the current state of the HashSet
        printHashSet(glavniHashSet);
    }

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

    // Create the persistent thread for client processing
    HANDLE client_thread = CreateThread(
        NULL,
        0,
        client_processing_thread,
        &sockfd,
        0,
        NULL
    );

    if (client_thread == NULL) {
        printf("Failed to create processing thread\n");
        closesocket(sockfd);
        WSACleanup();
        return 1;
    }

    // Main thread continues with other tasks
    printf("Main thread is now free for other operations.\n");

    // Wait for the processing thread to finish (optional)
    WaitForSingleObject(client_thread, INFINITE);
    CloseHandle(client_thread);

    // Cleanup resources
    closesocket(sockfd);
    WSACleanup();

    DeleteCriticalSection(&criticalSection);

    // Free the HashSet when done
    freeHashSet(glavniHashSet);
    free(glavniHashSet);

    return 0;
}