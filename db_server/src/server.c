#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/errno.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <stdio.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <syslog.h>
#include <fcntl.h>
#include <signal.h>

#include "../include/request.h"
#include "requestHandler.c"

#define terminate(str) perror(str); exit(EXIT_FAILURE);
#define BUFFERSZ 1024
#define NOARG 0
#define PORTARG 1
#define FILEARG 2

char databasePath[] = "../../database/";
volatile sig_atomic_t runServer = 1;
void serve(int port, char *logFile);
void freeChild();
void gracefulShutdown(int signum);
void daemonizeServer();

int main(int argc, char* argv[]) {
    int port = 1337, daemonize = 0, isNumber = 0, findCMDParam = NOARG;
    char *logFile = NULL;
    char usage[100] = "Usage: ";
    strcat(usage, argv[0]);
    strcat(usage, " [-p port] [-d] [-l logfile] [-h]\n");
    char helpStr[200] = "-p port - Listen to port with port number port\n-d - Run as a daemon as opposed to a program\n-l logfile - Log to file with name logfile (default is syslog)\n-h - Print help text\n";

    for (int i = 1; i < argc; i++) {
        if (findCMDParam == NOARG) {
            if (strcmp("-p", argv[i]) == 0) {
                if ((argc - 1) > i) {
                    findCMDParam = PORTARG;
                }
                else {
                    printf("ERROR: Missing argument parameter\n%s", usage);
                    exit(EXIT_FAILURE);
                }
            }
            else if (strcmp("-d", argv[i]) == 0) {
                daemonize = 1;
            }
            else if (strcmp("-h", argv[i]) == 0) {
                printf("%s\n%s", usage, helpStr);
                exit(EXIT_SUCCESS);
            }
            else if (strcmp("-l", argv[i]) == 0) {
                if ((argc - 1) > i) {
                    printf("Log file parameter found (");
                    findCMDParam = FILEARG;
                }
                else {
                    printf("ERROR: Missing argument parameter\n%s", usage);
                    exit(EXIT_FAILURE);
                }
            }
            else {
                printf("ERROR: Incorrect input parameter\n%s", usage);
                exit(EXIT_FAILURE);
            }
        }
        else if (findCMDParam > NOARG) {
            if (findCMDParam == PORTARG) {
                for (int j = 0; j < strlen(argv[i]); j++) {
                    if (isdigit(argv[i][j]) == 0) {
                        printf("ERROR: Invalid port number\n%s", usage);
                        exit(EXIT_FAILURE);
                    }
                }

                port = atoi(argv[i]);
                findCMDParam = NOARG;
            }
            else if (findCMDParam == FILEARG) {
                logFile = malloc(strlen(argv[i]));
                strcpy(logFile, argv[i]);
                printf("%s) (not yet implemented)\n", logFile);
                exit(EXIT_SUCCESS);
            }
        }
    }

    if (daemonize > 0) {
        printf("[+] Daemonizing server on port: %d  \n", port);
        daemonizeServer();
    }

    serve(port, logFile);
}

void serve(int port, char *logFile) {
    int serverSocket, clientSocket;
    request_t *request;
    char *error;
    pid_t pid;

    /* Signal for graceful shutdown of server instance */
    struct sigaction shutdownServer;
    shutdownServer.sa_handler = gracefulShutdown;
    sigemptyset(&shutdownServer.sa_mask);
    shutdownServer.sa_flags = 0;

     /* Signal for ignoring (freeing) terminated child processes */
    struct sigaction ignoreChild;
    ignoreChild.sa_handler = freeChild;
    sigemptyset(&ignoreChild.sa_mask);
    ignoreChild.sa_flags = SA_RESTART;

    char *receiveBuffer = NULL;

    char welcomeMessage[100] = "Welcome! Please provide your SQL query\n";
    
    if (port == 1337) {
        printf("[+] Server started on default port: %d (pid: %d)\n", port, getpid());
    }
    else {
        printf("[+] Server started on port: %d (pid: %d)\n", port, getpid());
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
    serverAddress.sin_port = htons(port);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    /* Binding server socket to specified address, port */
    if(bind(serverSocket, (struct sockaddr*) &serverAddress, sizeof(serverAddress)) == -1) {
        terminate("[-] Error occurred when binding server socket");
    }
    printf("[+] Socket binding to port %d was successful\n", port);

    /* Starting to listen on server socket */
    if(listen(serverSocket, 10) == -1) {
        terminate("[-] Error occurred when listening on server socket");
    }
    printf("[+] Server listening for incoming connections on port %d\n", port);

    /* Defining client address in sockaddr_in struct */
    struct sockaddr_in clientAddress;
    memset(&clientAddress, 0, sizeof(clientAddress));
    int addressLength = sizeof(clientAddress);
    char clientIP[INET_ADDRSTRLEN];

    while(runServer) {
        /* Signal for detecting CTRL + C - calls signal handler gracefulShutdown */
        sigaction(SIGINT, &shutdownServer, NULL);

        /* Accepting client connections - this is a blocking call */
        if((clientSocket = accept(serverSocket, (struct sockaddr*) &clientAddress, (socklen_t*) &addressLength)) == -1) {
            if (runServer) {
                terminate("[-] Error occurred when accepting new connection");
            }
        }
        else {
            inet_ntop(AF_INET, &clientAddress.sin_addr, clientIP, sizeof(clientIP));
            printf("[*] New connection from %s:%i accepted\n", clientIP, ntohs(clientAddress.sin_port));
        }

        if (runServer) {
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
                    memset(receiveBuffer, 0, BUFFERSZ);

                    /* Receiving data (SQL request) from client, placing in buffer */
                    if(recv(clientSocket, receiveBuffer, BUFFERSZ, 0) == -1){
                        if(runServer) {
                            terminate("[-] Error occurred when receiving data from client");
                        }
                        else {
                            send(clientSocket, "Bye-bye now! ¯\\_( ͡° ͜ʖ ͡°)_/¯\n", sizeof("Bye-bye now! ¯\\_( ͡° ͜ʖ ͡°)_/¯\n"), 0);
                            shutdown(clientSocket, SHUT_RDWR);
                            close(clientSocket);
                            exit(EXIT_SUCCESS);
                        }
                        
                    } /* If client closes terminal session */
                    else if (strlen(receiveBuffer) == 0){
                        printf("[*] Disconnected client %s:%i - client closed terminal session\n", clientIP, ntohs(clientAddress.sin_port));
                        
                        /* Close client socket */
                        shutdown(clientSocket, SHUT_RDWR);
                        close(clientSocket);

                        /* Free receive buffer memory */
                        free(receiveBuffer);

                        exit(EXIT_SUCCESS);
                    }
                    else {
                        receiveBuffer[strlen(receiveBuffer) - 2] = '\0';
                        printf("[*] Client %s:%i sent: %s (%ld byte(s))\n", clientIP, ntohs(clientAddress.sin_port), receiveBuffer, strlen(receiveBuffer));
                        request = parse_request(receiveBuffer, &error);
                        memset(receiveBuffer, 0, BUFFERSZ);

                        if (request != NULL && request->request_type == RT_QUIT) {
                            send(clientSocket, "Bye-bye now! ¯\\_( ͡° ͜ʖ ͡°)_/¯\n", sizeof("Bye-bye now! ¯\\_( ͡° ͜ʖ ͡°)_/¯\n"), 0);
                            printf("[*] Disconnected client %s:%i - client sent .quit command\n", clientIP, ntohs(clientAddress.sin_port));
                            /* Close client socket */
                            shutdown(clientSocket, SHUT_RDWR);
                            close(clientSocket);

                            /* Free receive buffer memory */
                            free(receiveBuffer);

                            /* Destroy request containing .quit command */
                            destroy_request(request);

                            exit(EXIT_SUCCESS);
                        }
                        else if (request == NULL) {
                            printf("[-] Invalid request received from %s:%i\n", clientIP, ntohs(clientAddress.sin_port));
                            printf("[-] Parser returned: %s\n", error);

                            /* If invalid request is not just an empty string, else... */
                            if (strcmp(error, "syntax error, unexpected $end") != 0) {
                                send(clientSocket, "ERROR: Invalid request sent\n", sizeof("ERROR: Invalid request sent\n"), 0);
                            }
                            else {
                                send(clientSocket, "Please provide a request\n", sizeof("Please provide a request\n"), 0);
                            }   

                            /* Free character array for error message from parser */
                            free(error);
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
                sigaction(SIGCHLD, &ignoreChild, NULL);
            }
        }
    }

    printf("\n[*] Server received shutdown signal - performing graceful shutdown\n");
    /* Shutting down server socket */
    printf("[*] Shutting down server socket\n");
    shutdown(serverSocket, SHUT_RDWR);
    
    /* Closing server socket */
    printf("[*] Closing server socket\n");
    close(serverSocket);

    /* Freeing memory */
    if (logFile != NULL) {
        printf("[*] Freeing allocated memory\n");
        free(logFile);
    }
}

void freeChild() {
    pid_t pid;
    if((pid = waitpid(-1, NULL, WNOHANG)) == -1){
        printf("[-] Problem occurred when terminating child (pid: %d)\n", pid);
    }
    else{
        printf("[+] Child was succesfully terminated (pid: %d)\n", pid);
    }
}

void gracefulShutdown(int signum) {
    runServer = 0;
}

void daemonizeServer() {
    int fileDesc0, fileDesc1, fileDesc2;
    pid_t pid;
    struct rlimit resourceLimit;
    struct sigaction signalAction;
    char pathBuffer[200];

    /* Clearing the file creation mask */
    umask(0);

    /* Retrieving CWD path */
    getcwd(pathBuffer, 200);

    /* Retrieving maximum number of FDs */
    if (getrlimit(RLIMIT_NOFILE, &resourceLimit) < 0) {
        perror("ERROR: Could not retreive maximum FDs\n");
        exit(EXIT_FAILURE);
    }

    pid = fork();
    if (pid < 0) {
        perror("ERROR: Could not fork\n");
        exit(EXIT_FAILURE);
    }
    else if (pid != 0) {
        exit(EXIT_SUCCESS);
    }

    if (setsid() < 0) {
        exit(EXIT_FAILURE);
    }

    signalAction.sa_handler = SIG_IGN;
    sigemptyset(&signalAction.sa_mask);
    signalAction.sa_flags = 0;

    if (sigaction(SIGHUP, &signalAction, NULL) < 0) {
        perror("ERROR: Could not ignore SIGHUP signal\n");
        exit(EXIT_FAILURE);
    }

    pid = fork();
    if (pid < 0) {
        perror("ERROR: Could not fork\n");
        exit(EXIT_FAILURE);
    }
    else if (pid != 0) {
        exit(EXIT_SUCCESS);
    }

    if (chdir(pathBuffer) < 0) {
        perror("Could not change directory");
        exit(EXIT_FAILURE);
    }

    if(resourceLimit.rlim_max == RLIM_INFINITY) {
        resourceLimit.rlim_max = 1024;
    }
    for (int i = 0; i < resourceLimit.rlim_max; i++) {
        close(i);
    }

    fileDesc0 = open("/dev/null", O_RDWR);
    fileDesc1 = dup(0);
    fileDesc2 = dup(0);
}