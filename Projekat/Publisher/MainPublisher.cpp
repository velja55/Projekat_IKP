#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>  // For inet_pton
#include "../Admin/globalVariable.cpp"

#define MAX_BUFFER_SIZE 1024         // Definiše maksimalnu veličinu bafera (1024 bajta) za prijem i slanje podataka.
#define SERVER_PORT 12345            // Definiše port na kojem server komunicira sa klijentima (UDP port 12345).

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
    while (!shutdown_variable) {
        //printf("Waiting for shutdown signal, current value of shutdown_variable: %d\n", shutdown_variable);
        Sleep(8000);  // Sleep for 1 second to check more frequently

        // Check if shutdown has occurred
        if (shutdown_variable) {
            break;
        }

        random++;
        pomocni = random % 10;  // %10 is the size of the random word array

        snprintf(buffer, sizeof(buffer), "operacija=2|publisher=stress_test|message=%s", RandomWords[pomocni]);

        bytes_sent = sendto(new_sockfd, buffer, strlen(buffer), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
        if (bytes_sent == SOCKET_ERROR) {
            printf("Failed to send message from publisher %d\n", 50000 + index);
            closesocket(new_sockfd);
            return 1;
        }

        // Setup for select() with a timeout to check every second
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
    HANDLE threads[2];  // Niz za rukovanje nitima
    int indices[2];     // Indeksi za niti
    int i;

    printf("Stress test started...\n");

    // Kreiranje 15 niti za slanje poruka
    for (i = 0; i < 2; i++) {
        indices[i] = i;  // Postavljanje indeksa za svaku nit
        threads[i] = CreateThread(NULL, 0, send_stress_message, &indices[i], 0, NULL);
        if (threads[i] == NULL) {
            printf("Failed to create thread for message %d\n", i);
            continue;
        }
    }

    // Čekanje da sve niti završe
    for (i = 0; i < 2; i++) {
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
    HANDLE threadHandle;

    printf("Choose an option:\n");
    printf("1. Normal message sending\n");
    printf("2. Stress test\n");
    printf("Enter your choice: ");
    scanf_s("%d", &choice);
    getchar();  // Uklanja '\n' iz input buffer-a

    if (choice == 1) {
        threadHandle = CreateThread(NULL, 0, send_message, &sockfd, 0, NULL);
    }
    else if (choice == 2) {
        threadHandle = CreateThread(NULL, 0, stressTest, &sockfd, 0, NULL);
    }
    else {
        printf("Invalid choice. Exiting.\n");
        closesocket(sockfd);
        WSACleanup();
        return 1;
    }

    if (threadHandle == NULL) {
        printf("Failed to create thread\n");
        closesocket(sockfd);
        WSACleanup();
        return 1;
    }

    WaitForSingleObject(threadHandle, INFINITE);
    CloseHandle(threadHandle);
    int neki;
    printf("Gasenje Publisher Programa \n");
    scanf_s("%d", &neki);
    closesocket(sockfd);
    WSACleanup();
    return 0;
}
