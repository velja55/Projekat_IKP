#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>  // For inet_pton

#define MAX_BUFFER_SIZE 1024         // Definiše maksimalnu veličinu bafera (1024 bajta) za prijem i slanje podataka.
#define SERVER_PORT 12345            // Definiše port na kojem server komunicira sa klijentima (UDP port 12345).

int main() {
    WSADATA wsaData;                    // Struktura koja sadrži informacije o Winsock verziji i drugim podešavanjima.

    // Pokreće Winsock biblioteku sa verzijom 2.2. Ako inicijalizacija ne uspe, izlazi sa greškom.
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup failed\n");            // Ako je inicijalizacija neuspešna, ispisuje grešku.
        exit(EXIT_FAILURE);                       // Izađe iz programa sa kodom greške.
    }

    SOCKET sockfd;                         // Definiše soket za komunikaciju (UDP socket).
    struct sockaddr_in server_addr;         // Struktura koja sadrži IP adresu i port servera.
    char buffer[MAX_BUFFER_SIZE];           // Bafer za prijem i slanje poruka.
    int bytes_received;                     // Varijabla za broj bajtova koje je klijent primio od servera.
    char message[MAX_BUFFER_SIZE];          // Bafer za naziv poruke.
    int max_size;                           // Maksimalna veličina.

    // Kreira UDP soket za komunikaciju sa serverom koristeći IPv4.
    sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sockfd == INVALID_SOCKET) {             // Ako soket nije uspešno kreiran, ispisuje grešku.
        printf("Socket creation failed\n");
        WSACleanup();                           // Čisti resurse Winsock-a.
        exit(EXIT_FAILURE);                     // Izađe iz programa sa greškom.
    }

    // Postavlja konfiguraciju servera
    memset(&server_addr, 0, sizeof(server_addr));  // Postavlja sve vrednosti u strukturi `server_addr` na 0.
    server_addr.sin_family = AF_INET;            // Postavlja tip adresiranja na IPv4.
    server_addr.sin_port = htons(SERVER_PORT);  // Postavlja port servera (koristi htons da bi se port konvertovao u mrežni redosled bajtova).

    // Pretvaranje IP adrese u binarni oblik.
    if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0) {
        printf("Invalid IP address\n");           // Ako IP adresa nije validna, ispisuje grešku.
        closesocket(sockfd);                     // Zatvara soket.
        WSACleanup();                            // Čisti resurse Winsock-a.
        exit(EXIT_FAILURE);                      // Izađe iz programa sa greškom.
    }

    // Startuje petlju koja omogućava višekratno slanje poruka serveru.
    while (1) {
        // Unos naziva poruke.
        printf("Enter the name of publisher to send to the server: ");
        fgets(message, sizeof(message), stdin);     // Unos naziva sa tastature.
        message[strcspn(message, "\n")] = '\0';     // Uklanja znak za novi red sa kraja stringa.

        // Ako korisnik unese 'exit', prekida petlju i zatvara klijenta.
        if (strcmp(message, "exit") == 0) {
            printf("Exiting client...\n");
            break;                                    // Izađe iz petlje i zatvori program.
        }

        // Unos maksimalne veličine.
        printf("Enter the max size: ");
        scanf_s("%d", &max_size);                      // Unos maksimalne veličine.
        getchar();                                    // Uklanja znak za novi red koji ostaje nakon unosa broja.

        // Kombinovanje naziva i maksimalne veličine u jedan string za slanje.
        snprintf(buffer, sizeof(buffer), "id=%d|naziv=%s|maxsize=%d", ntohs(((struct sockaddr_in*)&server_addr)->sin_port), message, max_size);

        // Šalje poruku serveru.
        sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));

        // Prijem odgovora od servera.
        bytes_received = recvfrom(sockfd, buffer, sizeof(buffer), 0, NULL, NULL);
        if (bytes_received == SOCKET_ERROR) {            // Ako prijem podataka ne uspe.
            printf("recvfrom failed\n");
            closesocket(sockfd);                        // Zatvara soket.
            WSACleanup();                               // Čisti resurse Winsock-a.
            exit(EXIT_FAILURE);                         // Izađe iz programa sa greškom.
        }

        buffer[bytes_received] = '\0';                   // Dodaje NULL karakter na kraj primljene poruke.
        printf("Received from server: %s\n", buffer);     // Ispisuje odgovor servera na ekran.
    }

    // Zatvara soket.
    closesocket(sockfd);
    WSACleanup();                                        // Čisti resurse Winsock-a.

    return 0;
}
