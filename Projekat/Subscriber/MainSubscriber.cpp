#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <WinSock2.h>
#include <windows.h>
#include <ws2tcpip.h>  // For inet_pton

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 12346
#define BUFFER_SIZE 1024

// Function to initialize WinSock
void initializeWinSock() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("Error: Failed to initialize WinSock.\n");
        exit(EXIT_FAILURE);
    }
}

// Function to handle communication with the server
// Function to handle communication with the server
void communicateWithServer(SOCKET serverSocket, sockaddr_in serverAddr) {
    char buffer[BUFFER_SIZE];
    int publisherID, choice;

    // Request the list of publishers from the server
    printf("Requesting list of publishers...\n");
    sendto(serverSocket, "get_publishers", strlen("get_publishers"), 0, (struct sockaddr*)&serverAddr, sizeof(serverAddr));

    // Receive the list of publishers
    int received = recvfrom(serverSocket, buffer, sizeof(buffer), 0, NULL, NULL);
    if (received <= 0) {
        printf("Error: Failed to receive data from server.\n");
        return;
    }
    buffer[received] = '\0';  // Null-terminate the string

    // Print available publishers
    printf("Available Publishers:\n%s\n", buffer);

    while (1) {
        // Let the user choose an action
        printf("Choose an option:\n");
        printf("1. Subscribe to a publisher\n");
        printf("2. Unsubscribe from a publisher\n");
        printf("3. Quit\n");
        printf("Enter your choice: ");
        scanf_s("%d", &choice);

        if (choice == 3) {
            break;
        }

        

        if (choice == 1) {
            printf("Enter the ID of the publisher: ");
            scanf_s("%d", &publisherID);
            // Send subscription request to the server
            sprintf_s(buffer, "subscribe:%d", publisherID);
            sendto(serverSocket, buffer, strlen(buffer), 0, (struct sockaddr*)&serverAddr, sizeof(serverAddr));

            // Receive the server's response
            received = recvfrom(serverSocket, buffer, sizeof(buffer), 0, NULL, NULL);
            if (received <= 0) {
                printf("Error: Failed to receive data from server.\n");
                return;
            }
            buffer[received] = '\0';  // Null-terminate the response

            // Print server response
            printf("Server Response: %s\n", buffer);

        }
        else if (choice == 2) {
            printf("Enter the ID of the publisher: ");
            scanf_s("%d", &publisherID);
            // Send unsubscription request to the server
            sprintf_s(buffer, "unsubscribe:%d", publisherID);
            sendto(serverSocket, buffer, strlen(buffer), 0, (struct sockaddr*)&serverAddr, sizeof(serverAddr));

            // Receive the server's response
            received = recvfrom(serverSocket, buffer, sizeof(buffer), 0, NULL, NULL);
            if (received <= 0) {
                printf("Error: Failed to receive data from server.\n");
                return;
            }
            buffer[received] = '\0';  // Null-terminate the response

            // Print server response
            printf("Server Response: %s\n", buffer);
        }
        else {
            printf("Invalid choice. Please try again.\n");
            continue;
        }

        

        if (choice == 1) {
            // Wait for the server's message (subscription)
            received = recvfrom(serverSocket, buffer, sizeof(buffer), 0, NULL, NULL);
            if (received <= 0) {
                printf("Error: Failed to receive message from server.\n");
                return;
            }
            buffer[received] = '\0';  // Null-terminate the message

            // Print the message from the server
            printf("Message from publisher: %s\n", buffer);
        }
    }
}


int main() {
    initializeWinSock();

    // Create a socket
    SOCKET clientSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (clientSocket == INVALID_SOCKET) {
        printf("Error: Failed to create socket.\n");
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    // Connect to the server
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));  // Postavlja sve vrednosti u strukturi `server_addr` na 0.
    serverAddr.sin_family = AF_INET;            // Postavlja tip adresiranja na IPv4.
    serverAddr.sin_port = htons(SERVER_PORT);  // Postavlja port servera (koristi htons da bi se port konvertovao u mrežni redosled bajtova).


    // Pretvaranje IP adrese u binarni oblik.
    if (inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr) <= 0) {
        printf("Invalid IP address\n");           // Ako IP adresa nije validna, ispisuje grešku.
        closesocket(clientSocket);                     // Zatvara soket.
        WSACleanup();                            // Čisti resurse Winsock-a.
        exit(EXIT_FAILURE);                      // Izađe iz programa sa greškom.
    }



    printf("Connected to the server.\n");

    // Communicate with the server
    communicateWithServer(clientSocket, serverAddr);

    // Clean up
    closesocket(clientSocket);
    WSACleanup();
    printf("Disconnected from the server.\n");

    return 0;
}
