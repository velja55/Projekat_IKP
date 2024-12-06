#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>  // For inet_ntop

#include "HashSetForSubscribers.h"
#include "Queue.h" 

#define MAX_BUFFER_SIZE 1024
#define PUBLISHER_PORT 12345

#define SUBSCRIBER_PORT 12346
#define THREAD_POOL_SIZE 4 
HashSet* glavniHashSet;
CRITICAL_SECTION criticalSection;           //ovaj kritical sektion msm da ni ne koristimo
Queue* queueZaPoruke;



// Struktura za prosleđivanje podataka nitima
typedef struct {
    SOCKET sockfd;
    struct sockaddr_in client_addr;
    int addr_len;
} client_data_t;


HANDLE workerThreads[THREAD_POOL_SIZE];  // Array to store thread handles

// Worker thread function for processing messages from the queue
DWORD WINAPI WorkerFunction(LPVOID lpParam) {
    while (1) {
        // Wait for the semaphore to be released (signal from the main thread)


  

        int publisherID;
        char* message = (char*)malloc(256 * sizeof(char));
        if (dequeue(queueZaPoruke, &publisherID, message) == 0) {
            // Find subscribers for the publisher in the HashSet
            LinkedList* subscribers = getSubscribers(glavniHashSet, publisherID);
            if (subscribers != NULL) {
                // Iterate through the LinkedList of subscribers
                Node* current = subscribers->head;
                while (current != NULL) {
                    SOCKET subscriberSocket = current->socket;  // Get the socket of the subscriber

                    printf("porukica: %s \n", message);
                    // Send the message to the subscriber
                    int bytesSent = sendto(
                        subscriberSocket,                       // UDP socket
                        message,                                // Message to send
                        strlen(message),                        // Length of the messagesadh
                        0,                                      // Flags
                        (struct sockaddr*)&(current->addr),     // Address of the subscriber
                        sizeof(current->addr)                   // Address length
                    );
                    if (bytesSent == SOCKET_ERROR) {
                        int error_code = WSAGetLastError();
                        printf("Failed to send message. Error code: %d\n", error_code);

                        // Opcionalno: Možete koristiti i switch-case za najčešće greške
                        switch (error_code) {
                        case WSAENOTCONN:
                            printf("Socket is not connected.\n");
                            break;
                        case WSAECONNRESET:
                            printf("Connection reset by peer.\n");
                            break;
                        case WSAEWOULDBLOCK:
                            printf("Socket is non-blocking and the operation would block.\n");
                            break;
                        case WSAEMSGSIZE:
                            printf("Message size is too large for the underlying protocol.\n");
                            break;
                        case WSAENETUNREACH:
                            printf("Network is unreachable.\n");
                            break;
                        default:
                            printf("Unknown error occurred.\n");
                            break;
                        }
                    }

                    current = current->next;  // Move to the next subscriber
                }
            }
            else {
                printf("No subscribers found for publisher ID %d\n", publisherID);
            }
        }

        free(message);  // Free memory allocated for the message
    }
    return 0;
}





DWORD WINAPI admin_console_thread(LPVOID arg) {
    int command;

    printf("Admin Console Started. Type a number for commands:\n");
    printf("  1 - List all active publishers\n");                                       //kada se doda publisher pise Operacija1 to je kontrola dole  u onom threadu ako je uspesno dodat nema veze sa ovim
    printf("  2 - Change Publishers Max Size for Subscribers\n");
    printf("  3 - Add Subscriber to a Publisher\n");                                    //vec postojeceg subscribera(da je na nekog subsribovan) dodajemo takodje drugom publisheru
    printf("  4 - Remove Subscriber from Publisher\n");
    printf("  5 - Shut down the server\n");

    while (1) {
        printf(">> ");

        // Read command and handle non-integer inputs
        if (scanf_s("%d", &command) != 1) {                                             //scanf_s ima problema ko unesemo slovo umesto broja za komandu
            // If scanf_s failed (invalid input like a letter), clear the input buffer and ask again
            while (getchar() != '\n'); // Clear the input buffer
            printf("Invalid input. Please enter a valid number (1, 2, or 3):\n");
            continue; // Skip this iteration and prompt the user again
        }

        if (command==1) {
            if (glavniHashSet->size == 0)
                printf("There are no Publishers \n");
            else
                printf("Listing all active publishers:\n");
                printHashSet(glavniHashSet); // A function you define to display publishers
        }
        else if (command == 2) {
            int publisherID = -1;
            int maxSize = -1;
            //NIJE URADJENO DA SE JAVI PUBLISHERU DA MU JE PROMENJEN MAX_SIZE
            printf("Enter Publisher's ID: ");
            while (scanf_s("%d", &publisherID) != 1 || publisherID < 0) {
                // Clear the invalid input
                while (getchar() != '\n');
                printf("Invalid input. Please enter a positive integer for Publisher's ID: ");
            }

            printf("Enter new size (0-100): ");
            while (scanf_s("%d", &maxSize) != 1 || maxSize < 0 || maxSize > 100) {
                // Clear the invalid input
                while (getchar() != '\n');
                printf("Invalid input. Please enter a number between 0 and 100 for size: ");
            }


            changeMaxSizeofSubscribers(glavniHashSet, publisherID, maxSize);
        }
        else if (command == 3)
        {
            int publisherID = -1;
            int subsriberID = -1;

            printf("Enter Subscriber's ID: ");
            while (scanf_s("%d", &subsriberID) != 1 || subsriberID < 0) {
                // Clear the invalid input
                while (getchar() != '\n');
                printf("Invalid input. Please enter a positive integer for subsriberID's ID: ");
            }

            printf("Enter Publisher's ID:");
            while (scanf_s("%d", &publisherID) != 1 || publisherID < 0) {
                // Clear the invalid input
                while (getchar() != '\n');
                printf("Invalid input. Please enter a positive integer for Publisher's ID: ");
            }

            //vec subsrajbovanog sub-a treba subsrajbovati na RAZLICITOG publishera

            //proverimo da li publisher na kog subsrajbujemo uopste postoji
            HashNode* pom = findPublisherNode(glavniHashSet, publisherID);

            if (pom == NULL)
            {
                printf("Publisher with that ID doesnt exist \n");
                continue;
            }

            //pretrazimo sve hashnodove i trazimo da li taj subscriber postoji
            HashNode* subsCurrentHashNode = findPublisherNodeBySubscriberID(glavniHashSet, subsriberID);

            if (subsCurrentHashNode == NULL)
            {
                printf("Subscriber with that ID doesnt exist\n");
                continue;
            }

            //ovde izvlacim socket od postojeceg suba- mozda je OVDE GRESKA MOZDA
            SOCKET subSocket = get(&subsCurrentHashNode->subscribers, subsriberID); //problem

            if (subSocket == INVALID_SOCKET)
            {
                printf("Subscriber with that ID doesnt exist \n");
                continue;

            }

            //treba nam i adresa od subscribera

            sockaddr_in subAddr = getAddr(&subsCurrentHashNode->subscribers, subsriberID);
            if (subAddr.sin_addr.s_addr == 0) {
                printf("Invalid address for subscriber ID %d\n", subsriberID);
                continue;
                    
            }

            //ovde je provera dal ga dodaje na isti publisher
            addSubscriber(glavniHashSet,publisherID, subsriberID, subSocket, subAddr);

        
        }
        else if (command == 4)
        {
            int publisherID = -1;
            int subsriberID = -1;
            printf("Enter Subscriber's ID: ");
            while (scanf_s("%d", &subsriberID) != 1 || subsriberID < 0) {
                // Clear the invalid input
                while (getchar() != '\n');
                printf("Invalid input. Please enter a positive integer for subsriberID's ID: ");
            }

            printf("Enter Publisher's ID:");
            while (scanf_s("%d", &publisherID) != 1 || publisherID < 0) {
                // Clear the invalid input
                while (getchar() != '\n');
                printf("Invalid input. Please enter a positive integer for Publisher's ID: ");
            }

            //ovo mi treba samo da bi poslao poruku posle subscriberu da je admin odjavio
            HashNode* pom = findPublisherNode(glavniHashSet, publisherID);

            if (pom == NULL)
            {
                printf("Publisher with that ID doesnt exist \n");
                continue;
            }

            //pretrazimo sve hashnodove i trazimo da li taj subscriber postoji
            HashNode* subsCurrentHashNode = findPublisherNodeBySubscriberID(glavniHashSet, subsriberID);

            if (subsCurrentHashNode == NULL)
            {
                printf("Subscriber with that ID doesnt exist\n");
                continue;
            }

            SOCKET subSocket = get(&subsCurrentHashNode->subscribers, subsriberID);

            if (subSocket == INVALID_SOCKET)
            {
                printf("Subscriber with that ID doesnt exist \n");
                continue;

            }

            //treba nam i adresa od subscribera

            sockaddr_in subAddr = getAddr(&subsCurrentHashNode->subscribers, subsriberID);
            if (subAddr.sin_addr.s_addr == 0) {
                printf("Invalid address for subscriber ID %d\n", subsriberID);
                continue;

            }
            int addrLen = sizeof(subAddr);

            
            sendto(subSocket, "Unsubscribed by ADMIN", strlen("Unsubscribed by ADMIN"), 0, (struct sockaddr*)&subAddr, addrLen);

            removeSubscriber(glavniHashSet, publisherID, subsriberID);              //dobro radi removovanje i dobro prezmu socket i adresu ali nece da se posalje poruka subssrajberu da je skinut

        }
        else if (command == 5) {                               //Ovo treba da uradimo tako da se svi threadovi notifikuje tipa sa globalnim bool-om, i onda kod workera npr while(shutdown!=true) da radi thread
            printf("Initiating graceful shutdown...\n");
            // Signal other threads to stop and clean up resources
            //initiate_shutdown(); // Define this function to handle cleanup
            
            
            //break; // Exit the loop to stop the thread
        }
        else {
            printf("Unknown command. Please type 1, 2, or 3 for available commands.\n");
            
        }
    }

    //return 0;
}




//DWORD WINAPI WorkerFunction(LPVOID lpParam) {
//    while (1) {
//        // Wait for the semaphore to be released (signal from the main thread)
//        WaitForSingleObject(hSemaphore, INFINITE);
//
//        // Process the message from the queue (ensure thread-safe access)
//        int publisherID;
//        char* message = (char*)malloc(256 * sizeof(char));
//        if (dequeue(queueZaPoruke, &publisherID, message) == 0) {
//            // Assuming sendToSubscribers sends the message to all subscribers
//           // sendToSubscribers(publisherID, message);  // Implement this function to send to subscribers
//            printf("Worker: Processing message from Publisher %d: %s\n", publisherID, message);
//        }
//        free(message);  // Free memory allocated for the message
//    }
//    return 0;
//}

// Funkcija za parsiranje poruke
// Function to parse the message and extract ID, naziv, and maxsize
void parse_message(char* buffer, int* operacija, int* id, size_t* maxSize, char* poruka, struct sockaddr_in* client_addr) {
    char id_str[16], maxsize_str[16], operacija_str[16];
    char* context;

    // Prvi token je operacija
    char* token = strtok_s(buffer, "|", &context);
    if (token != NULL) {
        sscanf_s(token, "operacija=%15s", operacija_str, (unsigned)_countof(operacija_str));
        *operacija = atoi(operacija_str);
    }

    // Drugi token je ID (koristi port klijenta)
    token = strtok_s(NULL, "|", &context);
    if (token != NULL) {
        sscanf_s(token, "id=%15s", id_str, (unsigned)_countof(id_str));
        *id = ntohs(client_addr->sin_port);
    }

    // Treći token je maxsize ili message
    token = strtok_s(NULL, "|", &context);
    if (*operacija == 1) {
        if (token != NULL) {
            sscanf_s(token, "maxsize=%15s", maxsize_str, (unsigned)_countof(maxsize_str));
            *maxSize = (size_t)atoi(maxsize_str);
        }
        poruka[0] = '\0';  // Nema poruke za operaciju 1
    }
    else {
        if (token != NULL) {
            // Čitanje cele poruke sa razmakom
            char* message_start = strstr(token, "message=");
            if (message_start != NULL) {
                strcpy_s(poruka, 256, message_start + 8);  // +8 preskače "message="
            }
            else {
                poruka[0] = '\0';  // Ako ne postoji poruka
            }
        }
        *maxSize = 0;
    }

    // Ispis primljenih detalja za proveru
    printf("Operacija: %d\n", *operacija);
    printf("ID (Port): %d\n", *id);
    if (*operacija == 1) {
        printf("MaxSize: %zu\n", *maxSize);
    }
    else {
        printf("Poruka: %s\n", poruka);
    }
}



DWORD WINAPI publisher_processing_thread(LPVOID arg) {
    SOCKET sockfd = *(SOCKET*)arg;
    struct sockaddr_in client_addr;
    int addr_len = sizeof(client_addr);
    char buffer[MAX_BUFFER_SIZE];
    int bytes_received;

    printf("Thread for processing client messages is running...\n");

    while (1) {
        // Receive message
        bytes_received = recvfrom(sockfd, buffer, MAX_BUFFER_SIZE, 0, (struct sockaddr*)&client_addr, &addr_len);
        if (bytes_received == SOCKET_ERROR) {
            printf("recvfrom failed\n");
            continue;
        }

        // Null-terminate the string
        buffer[bytes_received] = '\0';

        // Parse the message
        int flag;
        int publisherID;
        size_t maxSize;
        char* poruka = (char*)malloc(256 * sizeof(char));
        parse_message(buffer,&flag, &publisherID, &maxSize,poruka, &client_addr);

        if (flag == 1) {
            addPublisher(glavniHashSet, publisherID, maxSize);              //critical section je u funckiji
            printHashSet(glavniHashSet);
        }
        else {
            enqueue(queueZaPoruke, publisherID, poruka);
            printQueue(queueZaPoruke);
        }
        // Send acknowledgment back to the client
        sendto(sockfd, "Acknowledged", strlen("Acknowledged"), 0, (struct sockaddr*)&client_addr, addr_len);

        // Print the current state of the HashSet
        printHashSet(glavniHashSet);

        // Free dynamically allocated memory for the message (to avoid memory leaks)
        free(poruka);
    }

    return 0;
}

/*
//do ovde sve dobro uradi
void sendPublisherIDsToSubscriber(SOCKET subscriberSocket, int* publisherIDs, size_t count, struct sockaddr_in* clientAddr) {
    if (!publisherIDs || count == 0) {
        const char* errorMsg = "No publishers available";
        sendto(subscriberSocket, errorMsg, strlen(errorMsg), 0, (struct sockaddr*)clientAddr, sizeof(*clientAddr));
        return;
    }

    // Send the IDs as a single data packet
    int bytesSent = sendto(subscriberSocket, (char*)publisherIDs, count * sizeof(int), 0, (struct sockaddr*)clientAddr, sizeof(*clientAddr));  //ne posalje dobre vrednosti
    if (bytesSent == SOCKET_ERROR) {
        printf("Failed to send publisher IDs to subscriber\n");
    }
    else {
        printf("Sent %zu publisher IDs to subscriber\n", count);
    }
}
*/

void sendPublisherIDsToSubscriber(SOCKET subscriberSocket, int* publisherIDs, size_t count, struct sockaddr_in* clientAddr) {
    if (!publisherIDs || count == 0) {
        const char* errorMsg = "No publishers available";
        sendto(subscriberSocket, errorMsg, strlen(errorMsg), 0, (struct sockaddr*)clientAddr, sizeof(*clientAddr));
        return;
    }

    // Konvertovanje niza ID-ova u string (CSV format)
    char buffer[MAX_BUFFER_SIZE];
    int offset = 0;

    for (size_t i = 0; i < count; i++) {
        offset += snprintf(buffer + offset, MAX_BUFFER_SIZE - offset, "%d,", publisherIDs[i]);
        if (offset >= MAX_BUFFER_SIZE) {
            printf("Error: Buffer overflow while serializing publisher IDs\n");
            return;
        }
    }

    // Ukloni poslednji zarez
    if (count > 0) {
        buffer[offset - 1] = '\0';
    }

    // Pošalji serijalizovane podatke klijentu
    int bytesSent = sendto(subscriberSocket, buffer, strlen(buffer), 0, (struct sockaddr*)clientAddr, sizeof(*clientAddr));
    if (bytesSent == SOCKET_ERROR) {
        printf("Failed to send publisher IDs to subscriber\n");
    }
    else {
        printf("Sent %zu publisher IDs to subscriber: %s\n", count, buffer);
    }
}

DWORD WINAPI subscriber_processing_thread(LPVOID arg) {
    SOCKET subscriberSocket = *(SOCKET*)arg;
    struct sockaddr_in clientAddr;
    int addrLen = sizeof(clientAddr);
    char buffer[MAX_BUFFER_SIZE];
    int bytesReceived;

    while (1) {
        //if socket closed/ subscriber deleted -> 

        bytesReceived = recvfrom(subscriberSocket, buffer, MAX_BUFFER_SIZE, 0, (struct sockaddr*)&clientAddr, &addrLen);
        if (bytesReceived == SOCKET_ERROR) {
            if (WSAGetLastError() == WSAENOTSOCK)
                break;

            printf("Error: Failed to receive data from server. WSA Error Code: %d\n", WSAGetLastError());
            continue;
        }
        buffer[bytesReceived] = '\0';  // Null-terminate received data

        printf("Message received from Subscriber: %s\n", buffer);

        if (strncmp(buffer, "get_publishers", 14) == 0) {
            int* publisherIDs = getAllPublisherIDs(glavniHashSet);
            if (publisherIDs) {
                sendPublisherIDsToSubscriber(subscriberSocket, publisherIDs, glavniHashSet->size, &clientAddr);
                free(publisherIDs);
            }
            else {
                printf("Failed to allocate memory for publisher IDs\n");
            }
        }
        else if (strncmp(buffer, "subscribe:", 10) == 0) {
            int publisherID = atoi(buffer + 10);
            int subscriberID = ntohs(clientAddr.sin_port);
            if (addSubscriber(glavniHashSet, publisherID, subscriberID, subscriberSocket, clientAddr) == true) {
                sendto(subscriberSocket, "Subscribed", strlen("Subscribed"), 0, (struct sockaddr*)&clientAddr, addrLen);
                printf("Subscriber %d added to Publisher %d\n", subscriberID, publisherID);
            }
            else {
                sendto(subscriberSocket, "Not Able to Subscribe", strlen("Not Able to Subscribe"), 0, (struct sockaddr*)&clientAddr, addrLen);
                printf("Subscriber %d failed to Subscribe to Publisher %d\n", subscriberID, publisherID);

            }

        }
        else if (strncmp(buffer, "unsubscribe:", 12) == 0) {
            int publisherID = atoi(buffer + 12);
            int subscriberID = ntohs(clientAddr.sin_port);
            if (removeSubscriber(glavniHashSet, publisherID, subscriberID) == true)
            {
                sendto(subscriberSocket, "Unsubscribed", strlen("Unsubscribed"), 0, (struct sockaddr*)&clientAddr, addrLen);
                printf("Subscriber %d removed from Publisher %d\n", subscriberID, publisherID);
            }
            else {
                sendto(subscriberSocket, "Unable to unsubscribe", strlen("Unable to unsubscribe"), 0, (struct sockaddr*)&clientAddr, addrLen);
                printf("Subscriber %d was unable to unsubscribe from Publisher %d\n", subscriberID, publisherID);

            }

        }
        else {
            printf("Unknown subscriber request: %s\n", buffer);
        }
    }
    return 0;
}
void InitializeThreadPool() {
    for (int i = 0; i < THREAD_POOL_SIZE; i++) {
        workerThreads[i] = CreateThread(NULL, 0, WorkerFunction, NULL, 0, NULL);
    }
}

void CloseThreadPool() {


    // Wait for all worker threads to finish
    for (int i = 0; i < THREAD_POOL_SIZE; i++) {
        WaitForSingleObject(workerThreads[i], INFINITE);  // Wait for the thread to terminate
        CloseHandle(workerThreads[i]);                   // Close the thread handle
    }

    // Close semaphore handle

    printf("Thread pool closed and resources cleaned up.\n");
}

int main() {
    WSADATA wsaData;
    SOCKET publisherSocket;
    SOCKET subscriberSocket;
    struct sockaddr_in publisherAddr, subscriberAddr;

    InitializeCriticalSection(&criticalSection);

    // Initialize the HashSet
    glavniHashSet = (HashSet*)malloc(sizeof(HashSet));
    queueZaPoruke = (Queue*)malloc(sizeof(Queue));
    if (glavniHashSet == NULL || queueZaPoruke == NULL) {
        printf("Error: Memory allocation failed for glavniHashSet.\n");
        return EXIT_FAILURE;
    }
    initHashSet(glavniHashSet);
    initQueue(queueZaPoruke);

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup failed\n");
        return 1;
    }

    // Create a UDP socket
    publisherSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (publisherSocket == INVALID_SOCKET) {
        printf("Publisher Socket creation failed\n");
        WSACleanup();
        return 1;
    }

    subscriberSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (subscriberSocket == INVALID_SOCKET) {
        printf("Subrciber Socket creation failed\n");
        WSACleanup();
        return 1;
    }

    // Set up server configuration
    memset(&publisherAddr, 0, sizeof(publisherAddr));
    publisherAddr.sin_family = AF_INET;
    publisherAddr.sin_addr.s_addr = INADDR_ANY;
    publisherAddr.sin_port = htons(PUBLISHER_PORT);

    // Bind the socket to the address
    if (bind(publisherSocket, (struct sockaddr*)&publisherAddr, sizeof(publisherAddr)) == SOCKET_ERROR) {
        printf("Bind failed\n");
        closesocket(publisherSocket);
        WSACleanup();
        return 1;
    }
    printf("Server is running on port %d...\n", PUBLISHER_PORT);

    // Bind subscriber socket
    subscriberAddr.sin_family = AF_INET;
    subscriberAddr.sin_addr.s_addr = INADDR_ANY;
    subscriberAddr.sin_port = htons(SUBSCRIBER_PORT);

    if (bind(subscriberSocket, (struct sockaddr*)&subscriberAddr, sizeof(subscriberAddr)) == SOCKET_ERROR) {
        printf("Subscriber socket binding failed\n");
        closesocket(publisherSocket);
        closesocket(subscriberSocket);
        WSACleanup();
        return 1;
    }
    // Create the persistent thread for publisher processing
    HANDLE publisher_thread = CreateThread(
        NULL,
        0,
        publisher_processing_thread,
        &publisherSocket,
        0,
        NULL
    );
    //ne treba ovde cistiti threadPool jel se skroz dole inicijalizuje ovde bi trebalo ocistiti kriticnu sekciju i strukture
    if (publisher_thread == NULL) {
        printf("Failed to create processing thread\n");
        DeleteCriticalSection(&criticalSection);            //ovaj critical sectiom msm da nigde ne koristimo, vec imamo onaj unutar dictioanry vec napravljen
        freeHashSet(glavniHashSet);
        freeQueue(queueZaPoruke);
        free(glavniHashSet);
        free(queueZaPoruke);
        closesocket(publisherSocket);
        closesocket(subscriberSocket);
        WSACleanup();
        return 1;
    }
    // Wait for the processing thread to finish (optional)

    HANDLE subscriberThread = CreateThread(NULL, 0, subscriber_processing_thread, &subscriberSocket, 0, NULL);

    if (subscriberThread == NULL) {
        printf("Failed to create Subscriber thread\n");
        DeleteCriticalSection(&criticalSection);
        // Free the HashSet when done
        freeHashSet(glavniHashSet);
        freeQueue(queueZaPoruke);
        free(glavniHashSet);
        free(queueZaPoruke);
        closesocket(publisherSocket);
        closesocket(subscriberSocket);
        WSACleanup();
        return 1;
    }

    //thread za console interakciju sa adminom
    HANDLE adminThread = CreateThread(NULL, 0, admin_console_thread, NULL, 0, NULL);
    if (adminThread == NULL) {
        DeleteCriticalSection(&criticalSection);
        // Free the HashSet when done
        freeHashSet(glavniHashSet);
        freeQueue(queueZaPoruke);
        free(glavniHashSet);
        free(queueZaPoruke);
        closesocket(publisherSocket);
        closesocket(subscriberSocket);
        WSACleanup();        
        return 1;
    }

    InitializeThreadPool();
    //ovi waitovi cekaju dok se ne zavrse operacije threadova kada se svi zavrse onda se inicijalizuje cleanpu closeHandle itd...
    WaitForSingleObject(publisher_thread, INFINITE);
    WaitForSingleObject(subscriberThread, INFINITE);

    // Wait for admin thread to finish
    WaitForSingleObject(adminThread, INFINITE);

    CloseHandle(publisher_thread);
    CloseHandle(subscriberThread);
    CloseHandle(adminThread);
    CloseThreadPool();
    // Cleanup resources
    closesocket(publisherSocket);
    closesocket(subscriberSocket);
    WSACleanup();
    DeleteCriticalSection(&criticalSection);
    // Free the HashSet when done
    freeHashSet(glavniHashSet);
    freeQueue(queueZaPoruke);
    free(glavniHashSet);
    free(queueZaPoruke);


    return 0;
}
