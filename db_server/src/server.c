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
#include <time.h>

#include "../include/request.h"
#include "requestHandler.c"

#define BUFFERSZ 1024
#define NOARG 0
#define PORTARG 1
#define FILEARG 2

#define SUCCESS 0
#define ERROR 1
#define INFO 2

char *logPath = NULL;
char databasePath[] = "../../database/";
volatile sig_atomic_t runServer = 1;

void terminate(char *str);
void serve(int port);
void freeChild();
void gracefulShutdown(int signum);
void daemonizeServer();
void serverLog(char *msg, int type);
char *stringConcatenator(char* str1, char* str2, int num);

int main(int argc, char* argv[]) {
    int port = 1337, daemonize = 0, isNumber = 0, findCMDParam = NOARG;
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
                logPath = malloc(strlen(argv[i]) + strlen(".log") + 1);
                strcpy(logPath, argv[i]);
                strcat(logPath, ".log");
                findCMDParam = NOARG;
            }
        }
    }

    if (daemonize > 0) {
        printf("[+] Daemonizing server on port: %d  \n", port);
        daemonizeServer();
    }
    else {
        openlog("SQL Server", LOG_PID | LOG_NDELAY, LOG_USER);
    }

    serve(port);
}

void serve(int port) {
    int serverSocket, clientSocket;
    request_t *request;
    char *error;
    pid_t pid;

    struct timespec sleepTime1;
    sleepTime1.tv_sec = 0;
    sleepTime1.tv_nsec = 500000000;

    struct timespec sleepTime2;
    sleepTime2.tv_sec = 1;
    sleepTime2.tv_nsec = 0;

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
    char *tempStr1 = NULL, *tempStr2 = NULL;

    char welcomeMessage[100] = "Welcome! Please provide your SQL query\n";
    
    tempStr1 = stringConcatenator("Created parent server instance (pid: ", "", getpid());
    tempStr2 = stringConcatenator(tempStr1, ")", -1);
    serverLog(tempStr2, SUCCESS);
    free(tempStr1);
    free(tempStr2);

    nanosleep(&sleepTime1, NULL);
    
    if (port == 1337) {
        serverLog("Server started on default port: 1337", SUCCESS);
    }
    else {
        tempStr1 = stringConcatenator("Server started on port: ", "", port);
        serverLog(tempStr1, SUCCESS);
        free(tempStr1);
    }

    nanosleep(&sleepTime1, NULL);

    /* Creating IPV4 server socket using TCP (SOCK_STREAM) */
    if ((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        terminate("Error occurred when creating server socket: ");
    }
    serverLog("Socket was successfully created", SUCCESS);

    nanosleep(&sleepTime1, NULL);

    /* Defining server address in sockaddr_in struct */
    struct sockaddr_in serverAddress;
    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    /* Binding server socket to specified address, port */
    if(bind(serverSocket, (struct sockaddr*) &serverAddress, sizeof(serverAddress)) == -1) {
        terminate("Error occurred when binding server socket: ");
    }

    tempStr1 = stringConcatenator("Socket binding to port ", "", port);
    tempStr2 = stringConcatenator(tempStr1, " was succesful", -1);
    serverLog(tempStr2, SUCCESS);
    free(tempStr1);
    free(tempStr2);

    nanosleep(&sleepTime1, NULL);

    /* Starting to listen on server socket */
    if(listen(serverSocket, 10) == -1) {
        terminate("Error occurred when listening on server socket: ");
    }

    tempStr1 = stringConcatenator("Server listening for incoming connections on port ", "", port);
    serverLog(tempStr1, SUCCESS);
    free(tempStr1);

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
                terminate("Error occurred when accepting new connection: ");
            }
        }
        else {
            inet_ntop(AF_INET, &clientAddress.sin_addr, clientIP, sizeof(clientIP));

            tempStr1 = stringConcatenator("Accepted new connection - ", clientIP, -1);
            tempStr2 = stringConcatenator(tempStr1, ":", ntohs(clientAddress.sin_port));
            serverLog(tempStr2, INFO);
            free(tempStr1);
            free(tempStr2);
        }

        if (runServer) {
            /* Forking parent process into child process(es) with fork() in order to handle concurrent client connections */
            if((pid = fork()) == 0) {
                tempStr1 = stringConcatenator("Child process was spawned (pid: ", "", getpid());
                tempStr2 = stringConcatenator(tempStr1, ")", -1);
                serverLog(tempStr2, SUCCESS);
                free(tempStr1);
                free(tempStr2);


                /* Allocating memory for child receive buffer */
                receiveBuffer = malloc(BUFFERSZ);

                /* Child process closes the listening server socket */
                close(serverSocket);

                /* Sending welcome message to client */
                if((send(clientSocket, welcomeMessage, sizeof(welcomeMessage), 0)) == -1) {
                    terminate("Error occurred when sending payload to client: ");
                }

                /* Infinite loop to continuously receive data from client */
                while(1) {
                    /* Zeroing out receiving buffer */
                    memset(receiveBuffer, 0, BUFFERSZ);

                    /* Receiving data (SQL request) from client, placing in buffer */
                    if(recv(clientSocket, receiveBuffer, (BUFFERSZ - 1), 0) == -1){
                        if(runServer) {
                            terminate("Error occurred when receiving data from client: ");
                        }
                        else {
                            send(clientSocket, "Server shutting down. Bye-bye now! ¯\\_( ͡° ͜ʖ ͡°)_/¯\n", sizeof("Server shutting down. Bye-bye now! ¯\\_( ͡° ͜ʖ ͡°)_/¯\n"), 0);
                            shutdown(clientSocket, SHUT_RDWR);
                            close(clientSocket);

                            /* Free receive buffer memory */
                            tempStr1 = stringConcatenator("Freeing receiver buffer memory (pid: ", "", getpid());
                            tempStr2 = stringConcatenator(tempStr1, ")", -1);
                            serverLog(tempStr2, INFO);
                            free(tempStr1);
                            free(tempStr2);
                            
                            free(receiveBuffer);
                            exit(EXIT_SUCCESS);
                        }
                        
                    } /* If client closes terminal session */
                    else if (strlen(receiveBuffer) == 0){
                        tempStr1 = stringConcatenator("Disconnected client (client closed terminal session) - ", clientIP, -1);
                        tempStr2 = stringConcatenator(tempStr1, ":", ntohs(clientAddress.sin_port));
                        serverLog(tempStr2, 2);
                        free(tempStr1);
                        free(tempStr2);
                        
                        /* Close client socket */
                        shutdown(clientSocket, SHUT_RDWR);
                        close(clientSocket);

                        /* Free receive buffer memory */
                        free(receiveBuffer);

                        exit(EXIT_SUCCESS);
                    }
                    else {
                        receiveBuffer[strlen(receiveBuffer) - 2] = '\0';
                        //printf("[*] Client %s:%i sent: %s (%ld byte(s))\n", clientIP, ntohs(clientAddress.sin_port), receiveBuffer, strlen(receiveBuffer));
                        tempStr1 = stringConcatenator("Client sent ", receiveBuffer, -1);
                        tempStr2 = stringConcatenator(tempStr1, " (", (strlen(receiveBuffer) + 1));
                        free(tempStr1);
                        tempStr1 = stringConcatenator(tempStr2, " bytes) - ", -1);
                        free(tempStr2);
                        tempStr2 = stringConcatenator(tempStr1, clientIP, -1);
                        free(tempStr1);
                        tempStr1 = stringConcatenator(tempStr2, ":", ntohs(clientAddress.sin_port));
                        serverLog(tempStr1, INFO);
                        free(tempStr1);
                        free(tempStr2);

                        request = parse_request(receiveBuffer, &error);
                        memset(receiveBuffer, 0, BUFFERSZ);

                        if (request != NULL && request->request_type == RT_QUIT) {
                            send(clientSocket, "Bye-bye now! ¯\\_( ͡° ͜ʖ ͡°)_/¯\n", sizeof("Bye-bye now! ¯\\_( ͡° ͜ʖ ͡°)_/¯\n"), 0);

                            tempStr1 = stringConcatenator("Disconnected client (client sent .quit command) - ", clientIP, -1);
                            tempStr2 = stringConcatenator(tempStr1, ":", ntohs(clientAddress.sin_port));
                            serverLog(tempStr2, INFO);
                            free(tempStr1);
                            free(tempStr2);

                            /* Close client socket */
                            shutdown(clientSocket, SHUT_RDWR);
                            close(clientSocket);

                            /* Free receive buffer memory */
                            tempStr1 = stringConcatenator("Freeing receiver buffer memory (pid: ", "", getpid());
                            tempStr2 = stringConcatenator(tempStr1, ")", -1);
                            serverLog(tempStr2, INFO);
                            free(tempStr1);
                            free(tempStr2);
                            
                            free(receiveBuffer);

                            /* Destroy request containing .quit command */
                            destroy_request(request);

                            exit(EXIT_SUCCESS);
                        }
                        else if (request == NULL) {
                            tempStr1 = stringConcatenator("Invalid request received from client - ", clientIP, -1);
                            tempStr2 = stringConcatenator(tempStr1, ":", ntohs(clientAddress.sin_port));
                            serverLog(tempStr2, ERROR);
                            free(tempStr1);
                            free(tempStr2);

                            tempStr1 = stringConcatenator("Parser returned error: ", error, -1);
                            serverLog(tempStr1, ERROR);
                            free(tempStr1);

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
    printf("\n");
    serverLog("Server received shutdown signal - performing graceful shutdown", INFO);
    nanosleep(&sleepTime2, NULL);

    /* Shutting down server socket */
    serverLog("Shutting down server socket", SUCCESS);
    shutdown(serverSocket, SHUT_RDWR);
    nanosleep(&sleepTime2, NULL);

    /* Closing server socket */
    serverLog("Closing server socket", SUCCESS);
    close(serverSocket);
    nanosleep(&sleepTime2, NULL);

    /* Freeing memory */
    if (logPath != NULL) {
        serverLog("Freeing allocated memory", SUCCESS);
        nanosleep(&sleepTime2, NULL);
        free(logPath);
        logPath = NULL;
    }

     /* Closing connection to syslog server */
    serverLog("Closing server log", SUCCESS);
    closelog();
}

void freeChild() {
    pid_t pid;
    char *tempStr1, *tempStr2;

    if((pid = waitpid(-1, NULL, WNOHANG)) == -1){
        tempStr1 = stringConcatenator("Problem occurred when terminating child (pid: ", "", pid);
        tempStr2 = stringConcatenator(tempStr1, ")", -1);
        serverLog(tempStr2, 0);
        free(tempStr1);
        free(tempStr2);
    }
    else{
        tempStr1 = stringConcatenator("Child was succesfully terminated (pid: ", "", pid);
        tempStr2 = stringConcatenator(tempStr1, ")", -1);
        serverLog(tempStr2, 0);
        free(tempStr1);
        free(tempStr2);
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
        perror("ERROR: Could not retreive maximum FDs");
        exit(EXIT_FAILURE);
    }

    pid = fork();
    if (pid < 0) {
        perror("ERROR: Could not fork");
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
        perror("ERROR: Could not ignore SIGHUP signal");
        exit(EXIT_FAILURE);
    }

    pid = fork();
    if (pid < 0) {
        perror("ERROR: Could not fork");
        exit(EXIT_FAILURE);
    }
    else if (pid != 0) {
        exit(EXIT_SUCCESS);
    }

    if (chdir(pathBuffer) < 0) {
        perror("ERROR: Could not change directory");
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

    openlog("SQL Server Daemon", LOG_PID | LOG_NDELAY, LOG_USER);
}

void serverLog(char *msg, int type) {
    FILE* fp;
    time_t rawTime = time(NULL);
    struct tm *hrTime = localtime(&rawTime);
    char currentTime[25];
    strftime(currentTime, 25, "%Y-%m-%d %H:%M:%S", hrTime);

    if (type == 0) {
        syslog(LOG_NOTICE, "%s", msg);
        printf("%s [+] %s\n", currentTime, msg);
    }
    else if (type == 1) {
        syslog(LOG_ERR, "%s", msg);
        printf("%s [-] %s\n", currentTime, msg);
    }
    else if (type == 2) {
        syslog(LOG_INFO, "%s\n", msg);
        printf("%s [*] %s\n", currentTime, msg);
    }

    if (logPath != NULL) {
        fp = fopen(logPath, "a");
        if (type == 0) {
            fprintf(fp, "%s [+] %s\n", currentTime, msg);
        }
        else if (type == 1) {
            fprintf(fp, "%s [-] %s\n", currentTime, msg);
        }
        else if (type == 2) {
            fprintf(fp, "%s [*] %s\n", currentTime, msg);
        }
        fclose(fp);
    }
}

char *stringConcatenator(char *str1, char *str2, int num) {
    char *concatStr = NULL;
    char numStr[10];

    if (num == -1) {
        concatStr = malloc(strlen(str1) + strlen(str2) + 1);
        strcpy(concatStr, str1);
        strcat(concatStr, str2);
    }
    else {
        concatStr = malloc(strlen(str1) + strlen(str2) + 11);
        sprintf(numStr, "%d", num);
        strcpy(concatStr, str1);
        strcat(concatStr, str2);
        strcat(concatStr, numStr);
    }
    return concatStr;
}

void terminate (char *str) {
    char* fullError = stringConcatenator(str, strerror(errno), -1);
    serverLog(fullError, ERROR);
    free(fullError);
    exit(EXIT_FAILURE);
}