#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>

#define MAX_BUFFER_SIZE 1024
#define PORT 12345

// Struktura za prosleđivanje podataka nitima
typedef struct {
    SOCKET sockfd;
    struct sockaddr_in client_addr;
    int addr_len;
} client_data_t;

// Funkcija niti za obradu klijenta
DWORD WINAPI handle_client(LPVOID arg) {
    client_data_t* data = (client_data_t*)arg;
    char buffer[MAX_BUFFER_SIZE];
    int bytes_received;

    // Prijem podataka od klijenta
    bytes_received = recvfrom(data->sockfd, buffer, MAX_BUFFER_SIZE, 0, (struct sockaddr*)&data->client_addr, &data->addr_len);
    if (bytes_received == SOCKET_ERROR) {
        printf("recvfrom nije uspeo\n");
        free(data);
        return 1;
    }

    // Završavanje stringa
    buffer[bytes_received] = '\0';

    printf("Poruka primljena: %s\n", buffer);

    // Opcionalno: slanje odgovora klijentu
    const char* response = "Poruka primljena";
    sendto(data->sockfd, response, strlen(response), 0, (struct sockaddr*)&data->client_addr, data->addr_len);

    free(data);
    return 0;
}

int main() {
    WSADATA wsaData;
    SOCKET sockfd;
    struct sockaddr_in server_addr;

    // Inicijalizacija Winsock-a
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup nije uspeo\n");
        return 1;
    }

    // Kreiranje UDP soketa
    sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sockfd == INVALID_SOCKET) {
        printf("Kreiranje soketa nije uspelo\n");
        WSACleanup();
        return 1;
    }

    // Konfiguracija server adrese
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Povezivanje soketa sa adresom
    if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        printf("Bind nije uspeo\n");
        closesocket(sockfd);
        WSACleanup();
        return 1;
    }

    printf("Server je pokrenut na portu %d...\n", PORT);

    // Glavna petlja servera
    while (1) {
        client_data_t* data = (client_data_t*)malloc(sizeof(client_data_t));
        if (data == NULL) {
            printf("Alokacija memorije nije uspela\n");
            continue;
        }

        // Postavljanje osnovnih podataka za klijenta
        data->sockfd = sockfd;
        data->addr_len = sizeof(data->client_addr);

        // Kreiranje niti za obradu klijenta
        HANDLE thread_handle = CreateThread(
            NULL,               // Podrazumevana sigurnosna atributa
            0,                  // Podrazumevana veličina steka
            handle_client,      // Funkcija niti
            (LPVOID)data,       // Argument za nit
            0,                  // Bez posebnih atributa prilikom kreiranja
            NULL                // ID niti nije potreban
        );

        if (thread_handle == NULL) {
            printf("CreateThread nije uspeo\n");
            free(data);
        }
        else {
            CloseHandle(thread_handle);
        }
    }

    // Čišćenje resursa
    closesocket(sockfd);
    WSACleanup();
    return 0;
}