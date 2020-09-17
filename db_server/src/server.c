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
#define die(str) perror(str); exit(-1);
#define BUFFERSZ 512

char *serve(int port) {
    int serverPort;
    int serverSocket, clientSocket;
    char *receiveBuffer = malloc(BUFFERSZ);
    char welcomeMessage[100] = "Hello! You have succesfully connected to the server. Send your request :)\n";
    
    if (port == 0) {
        serverPort = 1337;
        printf("Server running on default port: %d\n", serverPort);
    }
    else {
        serverPort = port;
        printf("Server running on port: %d\n", serverPort);
    }

    /* Creating IPV4 server socket using TCP (SOCK_STREAM) */
    if ((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        die("socket");
    }

    /* Defining server address in sockaddr_in struct */
    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(serverPort);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    /* Binding server socket to specified address, port */
    if(bind(serverSocket, (struct sockaddr*) &serverAddress, sizeof(serverAddress)) == -1) {
        die("bind");
    }

    /* Starting to listen on server socket */
    if(listen(serverSocket, 10) == -1) {
        die("listen");
    }

    /* Defining client address in sockaddr_in struct */
    struct sockaddr_in clientAddress;
    int addressLength = sizeof(clientAddress);
    char clientIP[INET_ADDRSTRLEN];

    /* Accepting client connections */
    if((clientSocket = accept(serverSocket, (struct sockaddr*) &clientAddress, (socklen_t*) &addressLength)) == -1) {
        die("accept");
    }
    else {
        inet_ntop(AF_INET, &clientAddress.sin_addr, clientIP, sizeof(clientIP));
        printf("Accepted connection from %s on port %i\n", clientIP, ntohs(clientAddress.sin_port));
    }

    /* Sending welcome message to client */
    if((send(clientSocket, welcomeMessage, sizeof(welcomeMessage), 0)) == -1) {
        die("send");
    }

    /* Receiving data (SQL request) from client, placing in buffer */
    if(recv(clientSocket, receiveBuffer, BUFFERSZ, 0) == -1){
        die("recv");
    }
    else {
    printf("Client sent: %s\n", receiveBuffer);
    }

    /* Closing server socket */
    close(serverSocket);

    /* Returning receiver buffer (pointer to char array) */
    return receiveBuffer;
}