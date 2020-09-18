#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/errno.h>
#include <arpa/inet.h>
#include "../include/request.h"
#define terminate(str) perror(str); exit(-1);
#define BUFFERSZ 1024

void serve(int port) {
    int serverPort;
    int serverSocket, clientSocket;
    request_t *request;
    pid_t parentid;

    char *receiveBuffer = malloc(BUFFERSZ);
    memset(receiveBuffer, 0, sizeof(receiveBuffer));

    char welcomeMessage[100] = "Welcome! Please send your SQL query.\n";
    
    if (port == 0) {
        serverPort = 1337;
        printf("[+] Server started on default port: %d\n", serverPort);
    }
    else {
        serverPort = port;
        printf("[+] Server started on port: %d\n", serverPort);
    }

    /* Creating IPV4 server socket using TCP (SOCK_STREAM) */
    if ((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        terminate("[-] Error occurred when creating server socket");
    }
    printf("[+] Socket was successfully created\n");

    /* Defining server address in sockaddr_in struct */
    struct sockaddr_in serverAddress;
    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(serverPort);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    /* Binding server socket to specified address, port */
    if(bind(serverSocket, (struct sockaddr*) &serverAddress, sizeof(serverAddress)) == -1) {
        terminate("[-] Error occurred when binding server socket");
    }
    printf("[+] Socket binding to port %d was successful\n", serverPort);

    /* Starting to listen on server socket */
    if(listen(serverSocket, 10) == -1) {
        terminate("[-] Error occurred when listening on server socket");
    }
    printf("[+] Server listening for incoming connections on port %d\n", serverPort);

    /* Defining client address in sockaddr_in struct */
    struct sockaddr_in clientAddress;
    memset(&clientAddress, 0, sizeof(clientAddress));
    int addressLength = sizeof(clientAddress);
    char clientIP[INET_ADDRSTRLEN];

    while(1) {
        /* Accepting client connections - this is a blocking call */
        if((clientSocket = accept(serverSocket, (struct sockaddr*) &clientAddress, (socklen_t*) &addressLength)) == -1) {
            terminate("[-] Error occurred when accepting new connection");
        }
        else {
            inet_ntop(AF_INET, &clientAddress.sin_addr, clientIP, sizeof(clientIP));
            printf("[*] New connection from %s:%i accepted\n", clientIP, ntohs(clientAddress.sin_port));
        }
        /* Forking parent process into child process(es) with fork() in order to handle concurrent client connections */
        if((parentid = fork()) == 0) {
            /* Child process closes the listening server socket */
            close(serverSocket);

            /* Sending welcome message to client */
            if((send(clientSocket, welcomeMessage, sizeof(welcomeMessage), 0)) == -1) {
                terminate("[-] Error occurred when sending payload to client");
            }

            /* Infinite loop to continuously receive data from client */
            while(1) {
                /* Receiving data (SQL request) from client, placing in buffer */
                if(recv(clientSocket, receiveBuffer, BUFFERSZ, 0) == -1){
                    terminate("[-] Error occurred when receiving data from client");
                }
                else {
                    receiveBuffer[strlen(receiveBuffer) - 2] = '\0';
                    printf("[*] Client %s:%i sent: %s (%ld)\n", clientIP, ntohs(clientAddress.sin_port), receiveBuffer, strlen(receiveBuffer));
                    request = parse_request(receiveBuffer);
                    memset(receiveBuffer, 0, sizeof(receiveBuffer));

                    if (request != NULL && request->request_type == RT_QUIT) {
                        printf("[*] Disconnected client %s:%i\n", clientIP, ntohs(clientAddress.sin_port));
                        destroy_request(request);
                        exit(0);
                    }
                    else if (request == NULL)
                    {
                        printf("[-] Invalid request received from %s:%i\n", clientIP, ntohs(clientAddress.sin_port));
                    }
                    
                }
            }
        }
        close(clientSocket);
    }

    /* Closing server socket */
    close(serverSocket);
}