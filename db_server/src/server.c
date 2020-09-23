#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <stdio.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/errno.h>
#include <arpa/inet.h>

#include "../include/request.h"
#include "requestHandler.c"

#define terminate(str) perror(str); exit(-1);
#define BUFFERSZ 1024
const char databasePath[] = "../../database/";

void serve(int port);
void freeChild();

int main(int argc, char* argv[]) {

    /* If port argument is present */
    if (argc == 2) {
        serve(atoi(argv[1]));
    }
    else
    {
        serve(0);
    } 
}

void serve(int port) {
    int serverPort;
    int serverSocket, clientSocket;
    request_t *request;
    pid_t pid;

    char *receiveBuffer = NULL;

    char welcomeMessage[100] = "Welcome! Please provide your SQL query.\n";
    
    if (port == 0) {
        serverPort = 1337;
        printf("[+] Server started on default port: %d (pid: %d)\n", serverPort, getpid());
    }
    else {
        serverPort = port;
        printf("[+] Server started on port: %d (pid: %d)\n", serverPort, getpid());
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
        if((pid = fork()) == 0) {
            printf("[+] Child client process was spawned (pid: %d)\n", getpid());

            /* Allocating memory for child receive buffer */
            receiveBuffer = malloc(BUFFERSZ);

            /* Child process closes the listening server socket */
            close(serverSocket);

            /* Sending welcome message to client */
            if((send(clientSocket, welcomeMessage, sizeof(welcomeMessage), 0)) == -1) {
                terminate("[-] Error occurred when sending payload to client");
            }

            /* Infinite loop to continuously receive data from client */
            while(1) {
                /* Zeroing out receiving buffer */
                memset(receiveBuffer, 0, sizeof(receiveBuffer));

                /* Receiving data (SQL request) from client, placing in buffer */
                if(recv(clientSocket, receiveBuffer, BUFFERSZ, 0) == -1){
                    terminate("[-] Error occurred when receiving data from client");
                } /* If client closes terminal session */
                else if (strlen(receiveBuffer) == 0){
                    printf("[*] Disconnected client %s:%i - client closed terminal session\n", clientIP, ntohs(clientAddress.sin_port));
                    
                    /* Close client socket */
                    close(clientSocket);

                    /* Free receive buffer memory */
                    free(receiveBuffer);

                    exit(0);
                }
                else {
                    receiveBuffer[strlen(receiveBuffer) - 2] = '\0';
                    printf("[*] Client %s:%i sent: %s (%ld byte(s))\n", clientIP, ntohs(clientAddress.sin_port), receiveBuffer, strlen(receiveBuffer));
                    request = parse_request(receiveBuffer);
                    memset(receiveBuffer, 0, sizeof(receiveBuffer));

                    if (request != NULL && request->request_type == RT_QUIT) {
                        send(clientSocket, "Bye-bye now!\n", sizeof("Bye-bye now!\n"), 0);
                        printf("[*] Disconnected client %s:%i - client sent .quit command\n", clientIP, ntohs(clientAddress.sin_port));
                        /* Close client socket */
                        close(clientSocket);

                        /* Free receive buffer memory */
                        free(receiveBuffer);

                        /* Destroy request containing .quit command */
                        destroy_request(request);

                        exit(0);
                    }
                    else if (request == NULL)
                    {
                        printf("[-] Invalid request received from %s:%i\n", clientIP, ntohs(clientAddress.sin_port));
                        send(clientSocket, "ERROR: Invalid request sent\n", sizeof("ERROR: Invalid request sent\n"), 0);
                    }
                    else if (request != NULL) {
                        /* Send request to request handler to dispatch to proper function */
                        handleRequest(request, clientSocket);
                    }
                    
                }
            }
        }
        else {
            /* Parent closes client socket and continues to accept method to accept new connections */
            close(clientSocket);
            
            /* Parent calls signal handler freeChild to wait for children to exit (prevents zombie processes) */
            signal(SIGCHLD, freeChild);
        }
    }

    /* Closing server socket */
    close(serverSocket);
}

void freeChild() {
    pid_t pid;

    if((pid = wait(NULL)) == -1){
        printf("[-] Problem occurred when terminating child (pid: %d)\n", pid);
    }
    else{
        printf("[+] Child was succesfully terminated (pid: %d)\n", pid);
    }
}