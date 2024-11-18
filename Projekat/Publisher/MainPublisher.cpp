#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h> // Za inet_pton

#define MAX_BUFFER_SIZE 1024
#define SERVER_PORT 12345

int main() {
    WSADATA wsaData;
    SOCKET sockfd;
    struct sockaddr_in server_addr;
    char buffer[MAX_BUFFER_SIZE];

    // Inicijalizacija Winsock-a
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup nije uspeo\n");
        exit(EXIT_FAILURE);
    }

    // Kreiranje UDP soketa
    sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sockfd == INVALID_SOCKET) {
        printf("Kreiranje soketa nije uspelo\n");
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    // Konfiguracija adrese servera
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);

    // Pretvaranje IP adrese u binarni oblik
    if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0) {
        printf("Neispravna IP adresa\n");
        closesocket(sockfd);
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    // Slanje poruke serveru
    printf("Unesite poruku za slanje serveru: ");
    fgets(buffer, sizeof(buffer), stdin);
    buffer[strcspn(buffer, "\n")] = '\0'; // Uklanjanje znaka za novi red

    sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));

    // Prijem odgovora od servera
    int bytes_received = recvfrom(sockfd, buffer, sizeof(buffer), 0, NULL, NULL);
    if (bytes_received == SOCKET_ERROR) {
        printf("recvfrom nije uspeo\n");
        closesocket(sockfd);
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    buffer[bytes_received] = '\0'; // Završavanje primljene poruke
    printf("Primljeno od servera: %s\n", buffer);

    // Zatvaranje soketa
    closesocket(sockfd);
    WSACleanup();

    return 0;
}
