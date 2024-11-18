#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>

#define MAX_BUFFER_SIZE 1024
#define PORT 12345

// Structure to pass to the thread function
typedef struct {
    SOCKET sockfd;
    struct sockaddr_in client_addr;
    int addr_len;
} client_data_t;

// Thread function to handle client requests
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

    // Null-terminate the received data
    buffer[bytes_received] = '\0';

    printf("Received message: %s\n", buffer);

    // Optionally, send a response back to the client
    const char* response = "Message received";
    sendto(data->sockfd, response, strlen(response), 0, (struct sockaddr*)&data->client_addr, data->addr_len);

    free(data);
    return 0;
}

int main() {
    WSADATA wsaData;
    SOCKET sockfd;
    struct sockaddr_in server_addr, client_addr;
    int client_addr_len = sizeof(client_addr);

    // Initialize Windows Sockets (Winsock)
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

    // Prepare the server address structure
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

    printf("Server is listening on port %d...\n", PORT);

    // Main server loop to handle multiple clients
    while (1) {
        client_data_t* data = (client_data_t*)malloc(sizeof(client_data_t));
        if (data == NULL) {
            printf("Malloc failed\n");
            continue;
        }
        char buffer[MAX_BUFFER_SIZE]; // Declare buffer for receiving data

        // Receive message from a client
        int bytes_received = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr*)&client_addr, &client_addr_len);
        if (bytes_received == SOCKET_ERROR) {
            printf("recvfrom failed\n");
            free(data);
            continue;
        }

        // Initialize client data
        data->sockfd = sockfd;
        data->client_addr = client_addr;
        data->addr_len = client_addr_len;

        // Create a new thread to handle this client
        HANDLE thread_handle = CreateThread(
            NULL,               // Default security attributes
            0,                  // Default stack size
            handle_client,      // Thread function
            (LPVOID)data,       // Argument for the thread
            0,                  // No creation flags
            NULL                // Don't need the thread ID
        );

        if (thread_handle == NULL) {
            printf("CreateThread failed\n");
            free(data);
        }
        else {
            // Close the thread handle after it's created
            CloseHandle(thread_handle);
        }
    }

    // Clean up
    closesocket(sockfd);
    WSACleanup();
    return 0;
}
