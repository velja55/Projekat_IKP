#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <WinSock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <conio.h>
#include "../Admin/globalVariable.cpp"

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 12346
#define BUFFER_SIZE 1024

// globalna prom al samo za subscribera 
int keepReceiving = 1;

const int stressCount = 50;        //promeniti na vise kada se ispravi bug

int exit_stress_test = 0;         //lokalna glob prom za stress test subscribera

int programExit = 0;

typedef struct {
    SOCKET clientSocket;
    struct sockaddr_in serverAddr;
} ThreadArgs;



// Function to initialize WinSock
void initializeWinSock() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("Error: Failed to initialize WinSock.\n");
        exit(EXIT_FAILURE);
    }
}


// Function to handle continuous receiving of messages from publisher
DWORD WINAPI receiveMessages(LPVOID param) {
    SOCKET serverSocket = *(SOCKET*)param;
    char buffer[BUFFER_SIZE];
    int received;

    while (!programExit) { // Ovaj uslov će osigurati da petlja prestane kada programExit bude postavljen na 1
        // Receive the server's message
        received = recvfrom(serverSocket, buffer, sizeof(buffer), 0, NULL, NULL);
        if (received <= 0) {
            printf("Error: Failed to receive message from server.\n");
            return 0;
        }
        buffer[received] = '\0';  // Null-terminate the message

        // Print the message from the publisher
        printf("\nMessage from publisher: %s\n", buffer);

        if (strcmp(buffer, "EXIT") == 0) {
            printf("Shutdown signal received. Enter any key to shutdown program...\n");
            programExit = 1; // Signalizacija za kraj programa
            return 0;
        }
    }

    return 0;
}

// Function to handle communication with the server
DWORD WINAPI communicateWithServer(LPVOID args) {
    ThreadArgs* threadArgs = (ThreadArgs*)args; // Cast args to ThreadArgs pointer
    SOCKET serverSocket = threadArgs->clientSocket;
    struct sockaddr_in serverAddr = threadArgs->serverAddr;
    char buffer[BUFFER_SIZE];
    int publisherID,choice;

    // Request the list of publishers from the server
    printf("Requesting list of publishers...\n");
    sendto(serverSocket, "get_publishers", strlen("get_publishers"), 0, (struct sockaddr*)&serverAddr, sizeof(serverAddr));

    // Receive the list of publishers
    int received = recvfrom(serverSocket, buffer, sizeof(buffer), 0, NULL, NULL);
    if (received <= 0) {
        printf("Error: Failed to receive data from server.\n");
        return 1;
    }
    buffer[received] = '\0';  // Null-terminate the string

    // Print available publishers
    printf("Available Publishers:\n%s\n", buffer);

    // Start a separate thread for receiving messages continuously
    HANDLE receiverThread = CreateThread(NULL, 0, receiveMessages, &serverSocket, 0, NULL);
    if (receiverThread == NULL) {
        printf("Error: Failed to create receiver thread.\n");
        return 1;
    }

    printf("Choose an option:\n");
    printf("1. Subscribe to a publisher\n");
    printf("2. Unsubscribe from a publisher\n");
    printf("3. Quit\n");

    while (!programExit) { // Uslov za izlazak iz ove petlje
        if (programExit == 1) {
            return 1;
        }
        printf("Enter your choice:");
        scanf_s("%d", &choice);

        if (choice == 3 || programExit) {
            programExit = 1;
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
                return 1;
            }
            buffer[received] = '\0';  // Null-terminate the response

            // Print server response
            printf("Server Response: %s\n", buffer);

            if (strcmp(buffer, "EXIT") == 0) {
                programExit = 1;
                return 0;
            }

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
                return 1;
            }
            buffer[received] = '\0';  // Null-terminate the response

            // Print server response
            printf("Server Response: %s\n", buffer);

            if (strcmp(buffer, "EXIT") == 0) {
                programExit = 1;
                return 0;
            }

            if (strcmp(buffer, "Unsubscribed by ADMIN") == 0) {
                programExit = 1;
                return 1;
            }

        }
        else {
            printf("Invalid choice. Please try again.\n");
            continue;
        }
    }

    // Wait for the receiver thread to finish
    WaitForSingleObject(receiverThread, INFINITE);
    CloseHandle(receiverThread);
    return 0;
}


DWORD WINAPI stressTestClientThread(LPVOID param) {
    int publisherID = *(int*)param;
    struct sockaddr_in serverAddr;
    char buffer[BUFFER_SIZE];
    int received;

    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &serverAddr.sin_addr);

    SOCKET subscriberSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (subscriberSocket == INVALID_SOCKET) {
        printf("Failed to create socket for Publisher %d.\n", publisherID);
        return 1;
    }

    struct sockaddr_in clientAddr;
    memset(&clientAddr, 0, sizeof(clientAddr));
    clientAddr.sin_family = AF_INET;
    clientAddr.sin_addr.s_addr = INADDR_ANY;
    clientAddr.sin_port = htons(0);
    if (bind(subscriberSocket, (struct sockaddr*)&clientAddr, sizeof(clientAddr)) == SOCKET_ERROR) {
        printf("Failed to bind socket for Publisher %d.\n", publisherID);
        closesocket(subscriberSocket);
        return 1;
    }

    sprintf_s(buffer, "subscribe:%d", publisherID);
    sendto(subscriberSocket, buffer, strlen(buffer), 0, (struct sockaddr*)&serverAddr, sizeof(serverAddr));

    struct sockaddr_in localAddr;
    int addrLen = sizeof(localAddr);
    if (getsockname(subscriberSocket, (struct sockaddr*)&localAddr, &addrLen) == SOCKET_ERROR) {
        printf("Failed to get local address information for Publisher %d\n", publisherID);
    }
    else {
        printf("Subscriber socket bound to port: %d\n", ntohs(localAddr.sin_port));
    }

    fd_set readfds;
    struct timeval timeout = { 1, 0 };  // 1 second timeout

    while (exit_stress_test==0) {  // Check the shutdown variable periodically
        // Periodic check without blocking the select call

        FD_ZERO(&readfds);
        FD_SET(subscriberSocket, &readfds);

        int ret = select(0, &readfds, NULL, NULL, &timeout);
        if (ret == SOCKET_ERROR) {
            printf("select() error while waiting for messages\n");
            break;
        }

        if (ret > 0 && FD_ISSET(subscriberSocket, &readfds)) {
            received = recvfrom(subscriberSocket, buffer, sizeof(buffer), 0, NULL, NULL);
            if (received > 0) {
                buffer[received] = '\0';

                //ovo je jedini nacin da se subscriber ugasi jer uopste ne moze da vidi globalnu promenljivu kada se promeni
                if (strcmp(buffer, "EXIT") == 0)
                {
                    exit_stress_test = 1;                                                   //samo 1 subscriber ce primiti od admina da treba da se ugasi i onda on svim ostalim saopstava tako sto menja lokalnu globalnu promenljivu samo kod susbcribera
                    break;

                }

                printf("SubscriberID: %d Message from Publisher %d: %s\n", ntohs(localAddr.sin_port), publisherID, buffer);
            }
        }

        // If shutdown signal has been set, break out of the loop
        if (exit_stress_test==1) {
            break;
        }

        // Sleep to avoid tight looping
        Sleep(100);  // Check every 100 milliseconds
    }

    printf("Shutdown signal received, exiting thread for Publisher %d.\n", publisherID);
    closesocket(subscriberSocket);
    return 0;
}







void startStressTest(SOCKET serverSocket) {
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &serverAddr.sin_addr);

    char buffer[BUFFER_SIZE];

    // Dobavljanje liste publishere-a
    sprintf_s(buffer, "get_publishers");

    // Kreiranje novog socketa za inicijalni zahtev
    SOCKET tempSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (tempSocket == INVALID_SOCKET) {
        printf("Failed to create socket for initial request.\n");
        return;
    }

    // Slanje zahteva za listu publishera
    sendto(tempSocket, buffer, strlen(buffer), 0, (struct sockaddr*)&serverAddr, sizeof(serverAddr));

    int received = recvfrom(tempSocket, buffer, sizeof(buffer), 0, NULL, NULL);
    closesocket(tempSocket); // Zatvaranje socketa nakon operacije

    if (received <= 0) {
        printf("Failed to retrieve publisher list.\n");
        return;
    }

    buffer[received] = '\0'; // Null-terminate response
    printf("Publisher list: %s\n", buffer);

    // Parsiranje liste publishere-a
    int publisherIDs[100];                                                                  //pazi da kada budemo povecali broj publishera da nam ovo ne bude Greska sto je fiksno
    int publisherCount = 0;

    char* nextToken = NULL;
    char* token = strtok_s(buffer, ",", &nextToken);
    while (token != NULL && publisherCount < 100) {
        publisherIDs[publisherCount++] = atoi(token);
        token = strtok_s(NULL, ",", &nextToken);
    }

    if (publisherCount == 0) {
        printf("No publishers available for subscription.\n");
        return;
    }

    printf("Parsed %d publishers for subscription.\n", publisherCount);

    // Kreiranje niti za svaki publisher
    HANDLE threads[stressCount]; // Maksimalan broj niti
    for (int i = 0; i < stressCount; i++) {
        threads[i] = CreateThread(NULL, 0, stressTestClientThread, &publisherIDs[i%publisherCount], 0, NULL);
        if (threads[i] == NULL) {
            printf("Failed to create thread for Publisher %d.\n", publisherIDs[i]);
        }
    }

    // Čekanje da sve niti završe
    for (int i = 0; i < stressCount; i++) {
        if (threads[i] != NULL) {
            WaitForSingleObject(threads[i], INFINITE);
            CloseHandle(threads[i]);
        }
    }

    printf("Stress test completed.\n");

    return;
}


int main() {
    initializeWinSock();

    // Kreiranje soketa
    SOCKET clientSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (clientSocket == INVALID_SOCKET) {
        printf("Error: Failed to create socket.\n");
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    // Povezivanje na server
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);

    if (inet_pton(AF_INET, SERVER_IP, &serverAddr.sin_addr) <= 0) {
        printf("Invalid IP address.\n");
        closesocket(clientSocket);
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    printf("Connected to the server.\n");

    // Opcije za rad
    int choice;
    printf("Choose mode:\n");
    printf("1. Standard communication with server\n");
    printf("2. Stress test\n");
    printf("Enter your choice: ");
    scanf_s("%d", &choice);

    if (choice == 1) {
        printf("Starting standard communication...\n");
        // Create a thread for communication
        ThreadArgs args = { clientSocket, serverAddr };
        HANDLE hThread = CreateThread(NULL, 0, communicateWithServer, &args, 0, NULL);
        if (hThread == NULL) {
            printf("Error creating communication thread.\n");
            return 1;
        }
        WaitForSingleObject(hThread, INFINITE); // Wait for the thread to finish
        CloseHandle(hThread);
    }
    else if (choice == 2) {
        // Stres test
        printf("Starting stress test...\n");
        startStressTest(clientSocket); // Funkcija koja pokreće stres test
       // printf("Press enter to close");
       // scanf_s("%d", &choice);
    }
    else {
        printf("Invalid choice. Exiting.\n");
    }

    // Čišćenje resursa
    closesocket(clientSocket);
    WSACleanup();
    printf("Gasenje Subscribera.\n");

    int pomocna5353 = 0;
    scanf_s("%d", &pomocna5353);

    return 0;
}
