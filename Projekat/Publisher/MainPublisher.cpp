#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h> // For inet_pton

#define MAX_BUFFER_SIZE 1024
#define SERVER_PORT 12345

int main() {
    WSADATA wsaData;
    SOCKET sockfd;
    struct sockaddr_in server_addr;
    char buffer[MAX_BUFFER_SIZE];

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup failed\n");
        exit(EXIT_FAILURE);
    }

    // Create a UDP socket
    sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sockfd == INVALID_SOCKET) {
        printf("Socket creation failed\n");
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    // Prepare the server address structure
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);

    // Use inet_pton to convert the IP address
    if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0) {
        printf("Invalid IP address\n");
        closesocket(sockfd);
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    // Send a message to the server
    printf("Enter message to send to server: ");
    fgets(buffer, sizeof(buffer), stdin);
    buffer[strcspn(buffer, "\n")] = '\0'; // Remove trailing newline

    sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));

    // Receive response from the server
    int bytes_received = recvfrom(sockfd, buffer, sizeof(buffer), 0, NULL, NULL);
    if (bytes_received == SOCKET_ERROR) {
        printf("recvfrom failed\n");
        closesocket(sockfd);
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    buffer[bytes_received] = '\0'; // Null-terminate the received message
    printf("Received from server: %s\n", buffer);

    // Close the socket
    closesocket(sockfd);
    WSACleanup();

    return 0;
}
