#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>  // For inet_ntop

#define MAX_BUFFER_SIZE 1024
#define PORT 12345

// Struktura za prosleđivanje podataka nitima
typedef struct {
    SOCKET sockfd;
    struct sockaddr_in client_addr;
    int addr_len;
} client_data_t;

// Funkcija za parsiranje poruke
void parse_message(char* buffer, struct sockaddr_in* client_addr) {
    char* token;
    char id[16], naziv[MAX_BUFFER_SIZE], maxsize[16];
    char* context;  // Context za strtok_s

    // Prvi token je ID (port)
    token = strtok_s(buffer, "|", &context);
    if (token != NULL) {
        sscanf_s(token, "id=%s", id, sizeof(id));
    }

    // Drugi token je naziv
    token = strtok_s(NULL, "|", &context);
    if (token != NULL) {
        sscanf_s(token, "naziv=%s", naziv, sizeof(naziv));
    }

    // Treći token je maxsize
    token = strtok_s(NULL, "|", &context);
    if (token != NULL) {
        sscanf_s(token, "maxsize=%s", maxsize, sizeof(maxsize));
    }

    // Ispis informacija
    printf("Received message details:\n");
    printf("ID (Port) of sender: %s\n", id);
    printf("Naziv: %s\n", naziv);
    printf("Maxsize: %s\n", maxsize);

    // Ispisivanje informacija o izvoru poruke (IP adresa i port)
    char client_ip[INET_ADDRSTRLEN];
    if (inet_ntop(AF_INET, &client_addr->sin_addr, client_ip, sizeof(client_ip)) == NULL) {
        printf("inet_ntop failed\n");
    }
    else {
        printf("Message received from IP: %s, Port: %d\n", client_ip, ntohs(client_addr->sin_port));
    }
}

// Thread function to handle a client
DWORD WINAPI handle_client(LPVOID arg) {  //nit za prihvat publishera
    client_data_t* data = (client_data_t*)arg;
    char buffer[MAX_BUFFER_SIZE];
    int bytes_received;

    // Receiving data from the client
    bytes_received = recvfrom(data->sockfd, buffer, MAX_BUFFER_SIZE, 0, (struct sockaddr*)&data->client_addr, &data->addr_len);
    if (bytes_received == SOCKET_ERROR) {
        printf("recvfrom failed\n");
        free(data);
        return 1;
    }

    // Null-terminate the string
    buffer[bytes_received] = '\0';

    // Print received message and client information (IP and port)
    parse_message(buffer, &data->client_addr);

    // Sending the response to the client
    sendto(data->sockfd, buffer, strlen(buffer), 0, (struct sockaddr*)&data->client_addr, data->addr_len);

    free(data);
    return 0;
}

int main() {
    WSADATA wsaData;
    SOCKET sockfd;
    struct sockaddr_in server_addr;

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

    // Postavljamo konfiguraciju servera
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

    // Kreiramo petlju za višekratno prihvatanje poruka
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
            NULL,               // Default security attributes
            0,                  // Default stack size
            handle_client,      // Thread function
            (LPVOID)data,       // Argument for the thread
            0,                  // Default creation flags
            NULL                // Thread ID not needed
        );

        if (thread_handle == NULL) {
            printf("CreateThread failed\n");
            free(data);
        }
        else {
            CloseHandle(thread_handle);
        }
    }

    // Cleanup resources
    closesocket(sockfd);
    WSACleanup();
    return 0;
}
