﻿#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>  // For inet_pton
#include "../Admin/globalVariable.cpp"

#define MAX_BUFFER_SIZE 1024         // Definiše maksimalnu veličinu bafera (1024 bajta) za prijem i slanje podataka.
#define SERVER_PORT 12345            // Definiše port na kojem server komunicira sa klijentima (UDP port 12345).

volatile int exit_program = 0;  // Globalna promenljiva koja označava da treba da se izađe iz programa


const int stresCount = 50;
// Struktura za prosleđivanje parametara u nit
typedef struct {
    SOCKET sockfd;
    struct sockaddr_in server_addr;
    int maxSize;
    char publisherName[MAX_BUFFER_SIZE];
} ThreadParams;

//random reci koje publisher salje subscriberima
const char* RandomWords[10] = {
    "jabuka",
    "kuca",
    "covek",
    "prozor",
    "reka",
    "sunce",
    "knjiga",
    "automobil",
    "stolica",
    "grad"
};

// Funkcija za kontinuirano slanje novih poruka serveru     SEKVENCIJALNI PRISTUP
DWORD WINAPI send_message(LPVOID param) {
    ThreadParams* params = (ThreadParams*)param;
    SOCKET sockfd = params->sockfd;
    struct sockaddr_in server_addr = params->server_addr;
    char message[MAX_BUFFER_SIZE];
    char buffer[MAX_BUFFER_SIZE];
    int bytes_received;

    fd_set readfds;
    struct timeval timeout;

    // Konfiguracija servera
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0) {
        printf("Invalid IP address\n");
        closesocket(sockfd);
        return 1;
    }

    // Unos maksimalne veličine
    int max_size;
    printf("Enter the max size: ");
    scanf_s("%d", &max_size);
    getchar();  // Uklanja znak za novi red iz input buffer-a

    // Kombinovanje podataka u poruku za slanje
    snprintf(buffer, sizeof(buffer), "operacija=1|publisher=%s|maxsize=%d", message, max_size);

    // Slanje inicijalne poruke serveru
    int bytes_sent = sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if (bytes_sent == SOCKET_ERROR) {
        printf("sendto failed: %d\n", WSAGetLastError());
        closesocket(sockfd);
        return 1;
    }

    // Glavna petlja za slanje poruka
    while (true) {
        // Postavljanje `select` za proveru poruka od servera
        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);
        timeout.tv_sec = 0;  // Proveravajte na svakih 500 ms
        timeout.tv_usec = 500000;

        // Proverite dolazne poruke od servera
        int ret = select(0, &readfds, NULL, NULL, &timeout);
        if (ret == SOCKET_ERROR) {
            printf("select failed: %d\n", WSAGetLastError());
            break;
        }

        if (ret > 0 && FD_ISSET(sockfd, &readfds)) {
            bytes_received = recvfrom(sockfd, buffer, sizeof(buffer), 0, NULL, NULL);
            if (bytes_received > 0) {
                buffer[bytes_received] = '\0';
                if (strcmp(buffer, "EXIT") == 0) {
                    printf("EXIT message received from server. Terminating...\n");
                    exit_program = 1;  // Postavite exit_program na 1 kada server pošalje "EXIT"
                    break;
                }
                printf("Received from server: %s\n", buffer);
            }
        }

        if (exit_program) {  // Ako je exit_program postavljen, prekinite petlju
            break;
        }

        // Proverite unos korisnika
        printf("Enter a message to send (or 'exit' to quit): ");
        if (fgets(message, sizeof(message), stdin) != NULL) {
            message[strcspn(message, "\n")] = '\0';  // Remove newline character
            if (strcmp(message, "exit") == 0) {
                printf("Exiting message sender...\n");
                break;
            }

            // Kombinovanje podataka za slanje
            snprintf(buffer, sizeof(buffer), "operacija=2|publisher=%s|message=%s", params->publisherName, message);

            // Slanje poruke serveru
            bytes_sent = sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
            if (bytes_sent == SOCKET_ERROR) {
                printf("sendto failed: %d\n", WSAGetLastError());
                break;
            }
        }
    }

    return 0;
}



DWORD WINAPI send_stress_message(LPVOID param) {
    struct sockaddr_in server_addr, local_addr;
    SOCKET new_sockfd;
    char buffer[MAX_BUFFER_SIZE];
    int bytes_sent, bytes_received;
    int index = *((int*)param);  // Indeks poruke za svaki soket

    // Kreiranje novog soketa
    new_sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (new_sockfd == INVALID_SOCKET) {
        printf("Socket creation failed for message %d\n", index);
        return 1;
    }

    // Set socket to non-blocking mode
    u_long mode = 1;  // 1 for non-blocking, 0 for blocking
    if (ioctlsocket(new_sockfd, FIONBIO, &mode) != 0) {
        printf("Failed to set socket to non-blocking mode for message %d\n", index);
        closesocket(new_sockfd);
        return 1;
    }

    // Postavljanje lokalne adrese sa jedinstvenim portom
    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = INADDR_ANY;  // Automatski bira lokalnu IP adresu
    local_addr.sin_port = htons(50000 + index);  // Jedinstveni lokalni port za svaki soket

    if (bind(new_sockfd, (struct sockaddr*)&local_addr, sizeof(local_addr)) == SOCKET_ERROR) {
        printf("Bind failed for local port %d\n", 50000 + index);
        closesocket(new_sockfd);
        return 1;
    }

    // Konfiguracija server adrese
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0) {
        printf("Invalid server IP address\n");
        closesocket(new_sockfd);
        return 1;
    }

    // Priprema poruke
    snprintf(buffer, sizeof(buffer), "operacija=1|publisher=stress_test|maxsize=%d", index);

    // Slanje poruke serveru
    bytes_sent = sendto(new_sockfd, buffer, strlen(buffer), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if (bytes_sent == SOCKET_ERROR) {
        printf("Failed to send message %d\n", 50000 + index);
        closesocket(new_sockfd);
        return 1;
    }

    // Setup for select() with a timeout to check every second
    fd_set readfds;
    struct timeval timeout;
    int ret;

    timeout.tv_sec = 1;  // Timeout set to 1 second
    timeout.tv_usec = 0; // No microseconds

    // Prijem odgovora od servera (opcionalno)
    FD_ZERO(&readfds);  // Clear the readfds set
    FD_SET(new_sockfd, &readfds);  // Add the socket to the set

    ret = select(0, &readfds, NULL, NULL, &timeout);
    if (ret == SOCKET_ERROR) {
        printf("Select failed for message %d\n", index);
        closesocket(new_sockfd);
        return 1;
    }
    if (ret > 0 && FD_ISSET(new_sockfd, &readfds)) {
        // Socket is ready to read
        bytes_received = recvfrom(new_sockfd, buffer, sizeof(buffer), 0, NULL, NULL);
        if (bytes_received == SOCKET_ERROR) {
            printf("Failed to receive initial response for message %d\n", 50000 + index);
            closesocket(new_sockfd);
            return 1;
        }
        buffer[bytes_received] = '\0';
        printf("Response from server for message %d: %s\n", 50000 + index, buffer);
    }

    int random = 0;     //kao random brojac
    int pomocni = 0;

    // Check periodically every second for shutdown
    while (true) {
        Sleep(8000);  // Pauza od 8 sekundi
        //printf("SHOYT PUB %d\n",shutdown_variable);

        if (shutdown_variable) {
            break;  // Izlazak iz petlje kada promenljiva postane true
        }

       // printf("Shut down variable %d", shutdown_variable);
        random++;
        pomocni = random % 10;  // Izbor reči iz RandomWords niza

        snprintf(buffer, sizeof(buffer), "operacija=2|publisher=stress_test|message=%s", RandomWords[pomocni]);

        bytes_sent = sendto(new_sockfd, buffer, strlen(buffer), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
        if (bytes_sent == SOCKET_ERROR) {
            printf("Failed to send message from publisher %d\n", 50000 + index);
            closesocket(new_sockfd);
            return 1;
        }

        // Prijem odgovora
        FD_ZERO(&readfds);
        FD_SET(new_sockfd, &readfds);

        ret = select(0, &readfds, NULL, NULL, &timeout);
        if (ret == SOCKET_ERROR) {
            printf("Select failed for message %d\n", index);
            closesocket(new_sockfd);
            return 1;
        }
        if (ret > 0 && FD_ISSET(new_sockfd, &readfds)) {
            bytes_received = recvfrom(new_sockfd, buffer, sizeof(buffer), 0, NULL, NULL);
            if (bytes_received == SOCKET_ERROR) {
                printf("Failed to receive message for publisher %d\n", 50000 + index);
                closesocket(new_sockfd);
                return 1;
            }
            buffer[bytes_received] = '\0';
            printf("Response from server for message %d: %s\n", 50000 + index, buffer);
        }
    }

    printf("Shutting down Stress Test\n");
    closesocket(new_sockfd);  // Close the socket after sending the message
    return 0;
}



DWORD WINAPI stressTest(LPVOID param) {
    SOCKET sockfd = *(SOCKET*)param;  // Ne koristi se direktno ovde, ali se prosleđuje kao parametar
    //broj publishera
    HANDLE threads[stresCount];  // Niz za rukovanje nitima
    int indices[stresCount];     // Indeksi za niti
    int i;

    printf("Stress test started...\n");

    // Kreiranje 15 niti za slanje poruka
    for (i = 0; i < stresCount; i++) {
        indices[i] = i;  // Postavljanje indeksa za svaku nit
        threads[i] = CreateThread(NULL, 0, send_stress_message, &indices[i], 0, NULL);
        if (threads[i] == NULL) {
            printf("Failed to create thread for message %d\n", i);
            continue;
        }
    }

    // Čekanje da sve niti završe
    for (i = 0; i < stresCount; i++) {
        if (threads[i] != NULL) {
            WaitForSingleObject(threads[i], INFINITE);
            CloseHandle(threads[i]);
        }
    }

    printf("Stress test completed.\n");
    return 0;
}



int main() {
    WSADATA wsaData;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup failed\n");
        exit(EXIT_FAILURE);
    }

    SOCKET sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sockfd == INVALID_SOCKET) {
        printf("Socket creation failed\n");
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    int choice;
    HANDLE publisherThread;

    printf("Choose an option:\n");
    printf("1. Normal message sending\n");
    printf("2. Stress test\n");
    printf("Enter your choice: ");
    scanf_s("%d", &choice);
    getchar();  // Uklanja '\n' iz input buffer-a

    if (choice == 1) {
        publisherThread = CreateThread(NULL, 0, send_message, &sockfd, 0, NULL);
    }
    else if (choice == 2) {
        publisherThread = CreateThread(NULL, 0, stressTest, &sockfd, 0, NULL);
    }
    else {
        printf("Invalid choice. Exiting.\n");
        closesocket(sockfd);
        WSACleanup();
        return 1;
    }

    if (publisherThread == NULL) {
        printf("Failed to create thread\n");
        closesocket(sockfd);
        WSACleanup();
        return 1;
    }

    WaitForSingleObject(publisherThread, INFINITE);
    CloseHandle(publisherThread);
    int neki;

    closesocket(sockfd);
    WSACleanup();


    printf("Gasenje Publisher Programa \n");
    scanf_s("%d", &neki);
    return 0;
}
