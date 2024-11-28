#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>  // For inet_pton

#define MAX_BUFFER_SIZE 1024         // Definiše maksimalnu veličinu bafera (1024 bajta) za prijem i slanje podataka.
#define SERVER_PORT 12345            // Definiše port na kojem server komunicira sa klijentima (UDP port 12345).

// Struktura za prosleđivanje parametara u nit
typedef struct {
    SOCKET sockfd;
    struct sockaddr_in server_addr;
    int maxSize;
    char publisherName[MAX_BUFFER_SIZE];
} ThreadParams;

// Funkcija za kontinuirano slanje novih poruka serveru
DWORD WINAPI send_continuous_messages(LPVOID param) {
    ThreadParams* params = (ThreadParams*)param;
    SOCKET sockfd = params->sockfd;
    struct sockaddr_in server_addr = params->server_addr;
    char message[MAX_BUFFER_SIZE];
    char buffer[MAX_BUFFER_SIZE];
    int bytes_received;

    while (1) {
        printf("Enter a message to send (or 'exit' to quit): ");
        fgets(message, sizeof(message), stdin);
        message[strcspn(message, "\n")] = '\0';  // Uklanja novi red sa kraja

        if (strcmp(message, "exit") == 0) {
            printf("Exiting message sender...\n");
            break;  // Napušta petlju i završava nit
        }

        // Kombinovanje naziva i poruke u string za slanje
        snprintf(buffer, sizeof(buffer), "operacija=2|publisher=%s|message=%s", params->publisherName, message);

        // Slanje poruke serveru
        sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));

        // Prijem odgovora od servera
        bytes_received = recvfrom(sockfd, buffer, sizeof(buffer), 0, NULL, NULL);
        if (bytes_received == SOCKET_ERROR) {
            printf("recvfrom failed\n");
            break;
        }

        buffer[bytes_received] = '\0';  // Dodaje NULL karakter na kraj primljene poruke
        printf("Received from server: %s\n", buffer);
    }

    free(params); // Oslobađa memoriju alociranu za parametre
    return 0;
}

// Funkcija za slanje inicijalne poruke serveru
DWORD WINAPI send_message(LPVOID param) {
    SOCKET sockfd = *(SOCKET*)param;
    struct sockaddr_in server_addr;
    char buffer[MAX_BUFFER_SIZE];
    char message[MAX_BUFFER_SIZE];
    int max_size;
    int bytes_received;

    // Konfiguracija servera
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0) {
        printf("Invalid IP address\n");
        closesocket(sockfd);
        return 1;
    }

    // Unos naziva publisher-a
    printf("Enter the name of publisher to send to the server: ");
    fgets(message, sizeof(message), stdin);
    message[strcspn(message, "\n")] = '\0';

    // Ako je uneseno 'exit', zatvaramo klijenta
    if (strcmp(message, "exit") == 0) {
        printf("Exiting client...\n");
        return 1;
    }

    // Unos maksimalne veličine
    printf("Enter the max size: ");
    scanf_s("%d", &max_size);
    getchar();  // Uklanja znak za novi red iz input buffer-a

    // Kombinovanje podataka u poruku za slanje
    snprintf(buffer, sizeof(buffer), "operacija=1|publisher=%s|maxsize=%d", message, max_size);

    // Slanje inicijalne poruke serveru
    sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));

    // Prijem odgovora od servera
    bytes_received = recvfrom(sockfd, buffer, sizeof(buffer), 0, NULL, NULL);
    if (bytes_received == SOCKET_ERROR) {
        printf("recvfrom failed\n");
        closesocket(sockfd);
        return 1;
    }

    buffer[bytes_received] = '\0';
    printf("Received from server: %s\n", buffer);

    // Kreiranje nove niti za kontinuirano slanje poruka
    ThreadParams* params = (ThreadParams*)malloc(sizeof(ThreadParams));
    params->sockfd = sockfd;
    params->server_addr = server_addr;
    params->maxSize = max_size;
    strcpy_s(params->publisherName, message);

    HANDLE continuousThread = CreateThread(
        NULL, 0, send_continuous_messages, params, 0, NULL);

    if (continuousThread == NULL) {
        printf("Failed to create continuous message thread\n");
        free(params);
    }
    else {
        WaitForSingleObject(continuousThread, INFINITE);
        CloseHandle(continuousThread);
    }

    return 0;
}

int main() {
    WSADATA wsaData;                    // Struktura koja sadrži informacije o Winsock verziji i drugim podešavanjima.

    // Pokreće Winsock biblioteku sa verzijom 2.2. Ako inicijalizacija ne uspe, izlazi sa greškom.
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup failed\n");            // Ako je inicijalizacija neuspešna, ispisuje grešku.
        exit(EXIT_FAILURE);                       // Izađe iz programa sa kodom greške.
    }

    SOCKET sockfd;                         // Definiše soket za komunikaciju (UDP socket).
    // Kreira UDP soket za komunikaciju sa serverom koristeći IPv4.
    sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sockfd == INVALID_SOCKET) {             // Ako soket nije uspešno kreiran, ispisuje grešku.
        printf("Socket creation failed\n");
        WSACleanup();                           // Čisti resurse Winsock-a.
        exit(EXIT_FAILURE);                     // Izađe iz programa sa greškom.
    }

    // Kreiraj nit koja će slati poruke serveru
    HANDLE sendThread = CreateThread(
        NULL,                // Bez posebnih atributa
        0,                   // Veličina stoga niti (0 znači sistemska vrednost)
        send_message,        // Funkcija koja će biti pozvana unutar nove niti
        &sockfd,             // Parametar za nit (soket)
        0,                   // Flags (0 znači normalno pokretanje)
        NULL                 // Povratni identifikator niti
    );

    if (sendThread == NULL) {
        printf("Failed to create send thread\n");
        closesocket(sockfd);
        WSACleanup();
        return 1;
    }

    // Čekanje da se nit završi (nije obavezno, možeš dodati čekanje u zavisnosti od implementacije)
    WaitForSingleObject(sendThread, INFINITE);

    // Zatvara soket.
    closesocket(sockfd);
    WSACleanup();                                        // Čisti resurse Winsock-a.

    return 0;
}
