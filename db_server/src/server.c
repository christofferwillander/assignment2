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

#define WRITE 0
#define READ 1

#define UNLOCK 0
#define LOCK 1

/* File paths */
char *logPath = NULL, *errPath = NULL;
char databasePath[] = "../../database/";

/* Signal handler variables */
volatile sig_atomic_t runServer = 1;
volatile sig_atomic_t runChildren = 1;

/* Data structures for keeping track of child processes */
int *childArray = NULL;
int childCtr = 0;

/* Miscellaneous helper-functions */
char *stringConcatenator(char* str1, char* str2, int num);
int countCommands(char *buffer, char* *commands);

/* Signal handlers */
void freeChild();
void gracefulShutdown();
void timerHandler();

/* Daemonization */
void daemonizeServer();

/* File locking function */
void doLock(int fd, int lock, int lockType);
//void doReadLock(int fd, int lock);

/* General server and request processing functions */
void serve(int port);
void terminate(char *str);
request_t* *multipleRequests(char *buffer, char *clientAddr, int nrOfCommands, int clientSocket);

/* Functions for managing child processes */
void addChild(pid_t pid);
void removeChild(pid_t pid);
void killChild();
void terminateChildren();

/* Server logging functionality  */
void serverLog(char *msg, int type);

int main(int argc, char* argv[]) {
    int port = 1337, daemonize = 0, isNumber = 0, findCMDParam = NOARG;

    char usage[100] = "Usage: ";
    strcat(usage, argv[0]);
    strcat(usage, " [-p port] [-d] [-l logfile] [-h]\n");
    char helpStr[200] = "-p port - Listen to port with port number port\n-d - Run as a daemon as opposed to a program\n-l logfile - Log to file with name logfile (default is syslog)\n-h - Print help text\n";

    /* Checking input flags and parameters */
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

                errPath = malloc(strlen(argv[i]) + strlen(".err") + 1);
                strcpy(errPath, argv[i]);
                strcat(errPath, ".err");

                findCMDParam = NOARG;
            }
        }
    }

    /* If server is set to start as a daemon */
    if (daemonize > 0) {
        struct sockaddr_in serverAddress;
        memset(&serverAddress, 0, sizeof(serverAddress));
        serverAddress.sin_family = AF_INET;
        serverAddress.sin_port = htons(port);
        serverAddress.sin_addr.s_addr = INADDR_ANY;
        int serverSocket;

        if ((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
            perror("Error occurred when creating server socket");
            exit(EXIT_FAILURE);
        }

        /* Checking if port is available before daemonizing server */
        if(bind(serverSocket, (struct sockaddr*) &serverAddress, sizeof(serverAddress)) == -1) {
            perror("Error occurred when binding server socket");
            exit(EXIT_FAILURE);
        }
        else {
            close(serverSocket);
            printf("[+] Daemonizing server on port: %d - use kill -2 <pid>/kill -15 <pid> to terminate\n", port);
            daemonizeServer();
        }
    }
    else {
        /* Opening log for non-daemonized execution */
        openlog("SQL Server", LOG_PID | LOG_NDELAY, LOG_USER);
    }

    serve(port);
}

void serve(int port) {
    int serverSocket, clientSocket;
    char *error;
    pid_t pid;

    /* Sleep timers - used for a more graceful and slow startup/shutdown */
    struct timespec sleepTime1;
    sleepTime1.tv_sec = 0;
    sleepTime1.tv_nsec = 500000000;

    struct timespec sleepTime2;
    sleepTime2.tv_sec = 1;
    sleepTime2.tv_nsec = 0;

    /* Signal masks */
    sigset_t chld_unblock_mask, chld_block_mask, parent_mask;

    sigemptyset(&chld_unblock_mask);
    sigemptyset(&chld_block_mask);
    sigemptyset(&parent_mask);

    sigaddset(&chld_block_mask, SIGUSR1);

    /* Signal for graceful shutdown of server instance */
    struct sigaction shutdownServer;
    shutdownServer.sa_handler = gracefulShutdown;
    sigemptyset(&shutdownServer.sa_mask);
    shutdownServer.sa_flags = 0;

    /* Signal for ignoring SIGINT, SIGTERM in child processes */
    struct sigaction ignoreInt;
    ignoreInt.sa_handler = SIG_IGN;
    sigemptyset(&ignoreInt.sa_mask);
    ignoreInt.sa_flags = 0;

     /* Signal for ignoring (freeing) terminated child processes */
    struct sigaction ignoreChild;
    ignoreChild.sa_handler = freeChild;
    sigemptyset(&ignoreChild.sa_mask);
    ignoreChild.sa_flags = SA_RESTART;

    /* Signal for handling shutdown signal (SIGUSR1) in child processes */
    struct sigaction terminateChild;
    terminateChild.sa_handler = killChild;
    sigemptyset(&terminateChild.sa_mask);
    terminateChild.sa_flags = 0;

    /* Receiver buffer for each server instance */
    char *receiveBuffer = NULL;

    /* Temporary strings used for string concatenation (server log) */
    char *tempStr1 = NULL, *tempStr2 = NULL;

    /* For handling multiple requests at once */
    int nrOfCommands = 0;
    char* *commands = NULL;

    /* Request pointers */
    request_t *request = NULL;
    request_t* *requests = NULL;

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

    /* Setting parent signal mask (empty) */
    if(sigprocmask(SIG_SETMASK, &parent_mask, NULL) < 0) {
        terminate("Error occured when setting parent signal mask: ");
    }
    while(runServer) {
        /* Signal for detecting CTRL + C or SIGTERM (through e.g. kill) - calls signal handler gracefulShutdown */
        if ((sigaction(SIGINT, &shutdownServer, NULL) < 0) || (sigaction(SIGTERM, &shutdownServer, NULL) < 0)) {
            terminate("Error occurred when setting parent signal action (SIGINT, SIGTERM): ");
        }

        /* Signal for waiting for children to exit (prevents zombie processes) */
        if (sigaction(SIGCHLD, &ignoreChild, NULL) < 0) {
            terminate("Error occurred when setting parent signal action (SIGCHLD): ");
        }

        /* Accepting client connections - this is a blocking call */
        if((clientSocket = accept(serverSocket, (struct sockaddr*) &clientAddress, (socklen_t*) &addressLength)) == -1) {
            if (errno != EINTR) {
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
                
                if ((sigaction(SIGINT, &ignoreInt, NULL) < 0) || (sigaction(SIGTERM, &ignoreInt, NULL) < 0)) {
                    terminate("Error occurred when setting child signal action (SIGINT, SIGTERM): ");
                }
                
                if(sigprocmask(SIG_SETMASK, &chld_unblock_mask, NULL) < 0) {
                    terminate("Error occurred when setting unblock signal mask: ");
                }

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
                    if(sigaction(SIGUSR1, &terminateChild, NULL) < 0){
                        terminate("Error occurred when setting child signal action: ");
                    }

                    /* Zeroing out receiving buffer */
                    memset(receiveBuffer, 0, BUFFERSZ);

                    if(runChildren) {
                        /* Receiving data (SQL request) from client, placing in buffer */
                        if(recv(clientSocket, receiveBuffer, (BUFFERSZ - 1), 0) == -1){
                            if (errno != EINTR) {
                                terminate("Error occurred when receiving data from client: ");
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
                        else { /* If data was successfully received from client */
                            /* Inserting null terminator */
                            receiveBuffer[strlen(receiveBuffer) - 2] = '\0';

                            /* Server logging */
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

                            /* Counting number of commands sent (for handling multiple requests at once) */
                            nrOfCommands = countCommands(receiveBuffer, commands);

                            /* If only one command was sent - parse immediately */
                            if (nrOfCommands >= 0 && nrOfCommands <= 1) {
                                request = parse_request(receiveBuffer, &error);
                            }
                            else if (nrOfCommands > 1) { /* Else, if multiple requsts were received... */
                                tempStr1 = stringConcatenator(clientIP,":", ntohs(clientAddress.sin_port));
                                requests = multipleRequests(receiveBuffer, tempStr1, nrOfCommands, clientSocket);
                            }

                            /* Zeroing out receiver buffer after processing data */
                            memset(receiveBuffer, 0, BUFFERSZ);
                            
                            /* If client sent .quit - terminate session gracefully */
                            if (request != NULL && request->request_type == RT_QUIT && nrOfCommands < 2) {
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
                            } /* If parser returned an error */
                            else if (request == NULL && nrOfCommands < 2) {
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
                            else if (request != NULL) { /* Else, if request was successfully parsed */
                                /* Set signal mask to block set signals during I/O operations */
                                if(sigprocmask(SIG_SETMASK, &chld_block_mask, NULL) < 0) {
                                    terminate("Error occurred when setting block signal mask: ");
                                }

                                /* Send request to request handler to dispatch to proper function */
                                handleRequest(request, clientSocket);
                                request = NULL;

                                /* Remove blocking of set signals */
                                if(sigprocmask(SIG_SETMASK, &chld_unblock_mask, NULL) < 0) {
                                    terminate("Error occurred when setting unblock signal mask: ");
                                }
                            }
                            else if (requests != NULL) { /* Else, if (multiple) requests were successfully parsed */
                                /* Set signal mask to block set signals during I/O operations */
                                if(sigprocmask(SIG_SETMASK, &chld_block_mask, NULL) < 0) {
                                    terminate("Error occurred when setting block signal mask: ");
                                }

                                /* Process requests - one by one */
                                for (int i = 0; i < nrOfCommands; i++) {
                                    handleRequest(requests[i], clientSocket);
                                }

                                free (requests);
                                requests = NULL;

                                /* Remove blocking of set signals */
                                if(sigprocmask(SIG_SETMASK, &chld_unblock_mask, NULL) < 0) {
                                    terminate("Error occurred when setting unblock signal mask: ");
                                }
                            }
                            
                        }
                    }
                    else {
                        /* Signal to shut down child instance was received */

                        send(clientSocket, "Server is shutting down. Bye-bye now! ¯\\_( ͡° ͜ʖ ͡°)_/¯\n", sizeof("Server is shutting down. Bye-bye now! ¯\\_( ͡° ͜ʖ ͡°)_/¯\n"), 0);
                        
                        /* Shutdown and close client socket */
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
                }
            }
            else {
                /* Add child to list */
                addChild(pid);

                /* Parent closes client socket and continues to accept method to accept new connections */
                close(clientSocket);
            }
        }
    }
    /* Signal to stop main server instance was received (parent) */
    write(STDOUT_FILENO, "\n", 2);
    serverLog("Server received shutdown signal - performing graceful shutdown", INFO);
    nanosleep(&sleepTime2, NULL);

    /* Call terminateChildren to signal shutdown operation to child processes */
    serverLog("Terminating child processes", INFO);
    terminateChildren();
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
    if (logPath != NULL && errPath != NULL) {
        serverLog("Freeing allocated memory", SUCCESS);
        nanosleep(&sleepTime2, NULL);

        free(logPath);
        logPath = NULL;

        free(errPath);
        errPath = NULL;
    }

     /* Closing connection to syslog server */
    serverLog("Closing server log", SUCCESS);
    closelog();
}

void freeChild() {
    int status = 0, curErr = errno;
    pid_t pid;
    char *tempStr1, *tempStr2;

    while((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        removeChild(pid);
        tempStr1 = stringConcatenator("Child was succesfully terminated (pid: ", "", pid);
        tempStr2 = stringConcatenator(tempStr1, ")", -1);
        serverLog(tempStr2, 0);
        free(tempStr1);
        free(tempStr2);
    }

    errno = curErr;
}

void gracefulShutdown() {
    int curErr = errno;
    runServer = 0;
    errno = curErr;
}

void daemonizeServer() {
    char *tempStr1 = NULL;
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
    tempStr1 = stringConcatenator("Server daemon succesfully started - pid: ", "", getpid());
    serverLog(tempStr1, SUCCESS);
    free(tempStr1);
}

void serverLog(char *msg, int type) {
    FILE* fp;

    time_t rawTime = time(NULL);
    struct tm *hrTime = localtime(&rawTime);
    char currentTime[25];
    char *buf = NULL;

    strftime(currentTime, 25, "%Y-%m-%d %H:%M:%S", hrTime);

    /* Logging to syslog and to terminal */
    if (type == 0) {
        syslog(LOG_NOTICE, "%s", msg);

        buf = malloc(strlen(currentTime) + strlen(" [+] ") + strlen(msg) + strlen("\n") + 1);
        sprintf(buf, "%s [+] %s\n", currentTime, msg);
        write(STDOUT_FILENO, &buf[0], strlen(buf) + 1);
        free(buf);
        buf = NULL;
    }
    else if (type == 1) {
        syslog(LOG_ERR, "%s", msg);
        
        buf = malloc(strlen(currentTime) + strlen(" [-] ") + strlen(msg) + strlen("\n") + 1);
        sprintf(buf, "%s [-] %s\n", currentTime, msg);
        write(STDOUT_FILENO, &buf[0], strlen(buf) + 1);
        free(buf);
        buf = NULL;
    }
    else if (type == 2) {
        syslog(LOG_INFO, "%s\n", msg);
        
        buf = malloc(strlen(currentTime) + strlen(" [*] ") + strlen(msg) + strlen("\n") + 1);
        sprintf(buf, "%s [*] %s\n", currentTime, msg);
        write(STDOUT_FILENO, &buf[0], strlen(buf) + 1);
        free(buf);
        buf = NULL;
    }

    /* Logging to file */
    if (logPath != NULL && errPath != NULL) {
        if (type == SUCCESS) {
            fp = fopen(logPath, "a");
            fprintf(fp, "%s [+] %s\n", currentTime, msg);
        }
        else if (type == ERROR) {
            fp = fopen(errPath, "a");
            fprintf(fp, "%s [-] %s\n", currentTime, msg);
        }
        else if (type == INFO) {
            fp = fopen(logPath, "a");
            fprintf(fp, "%s [*] %s\n", currentTime, msg);
        }
        fclose(fp);
    }
}

char *stringConcatenator(char *str1, char *str2, int num) {
    char *concatStr = NULL;
    char numStr[10];

    /* If no number placeholder is present in string */
    if (num == -1) {
        concatStr = malloc(strlen(str1) + strlen(str2) + 1);
        strcpy(concatStr, str1);
        strcat(concatStr, str2);
    }
    else { /* Else if number is to be inserted (in end of string) */
        concatStr = malloc(strlen(str1) + strlen(str2) + 11);
        sprintf(numStr, "%d", num);
        strcpy(concatStr, str1);
        strcat(concatStr, str2);
        strcat(concatStr, numStr);
    }

    return concatStr;
}

void terminate (char *str) {
    /* Retreiving error string */
    char* fullError = stringConcatenator(str, strerror(errno), -1);

    /* Logging to server log */
    serverLog(fullError, ERROR);
    free(fullError);

    /* Terminating */
    exit(EXIT_FAILURE);
}

int countCommands(char *buffer, char* *commands) {
    char findStr[2] = ";";
    int count = 0;
    char *curPos = buffer;

    if(buffer[0] == '.') {
        count++;
    }
    else if (strcmp(buffer, "\n") == 0) {
    }
    else {
        while (strstr(curPos, findStr) != NULL) {
            curPos = strstr(curPos, findStr) + 1;
            count++;
        }
    }

    return count;
}

request_t* *multipleRequests(char *buffer, char *clientAddr, int nrOfCommands, int clientSocket) {
    char *temp1 = NULL, *temp2 = NULL;
    char *tempStr1 = NULL, *tempStr2 = NULL;
    char *error = NULL;
    int length = 0;
    request_t* *requests = NULL;
    requests = malloc(sizeof(request_t*) * nrOfCommands);

    for (int i = 0; i < nrOfCommands; i++) {
        temp1 = strstr(buffer, ";");
        length = (int)(temp1 - buffer) + 2;

        temp2 = malloc(length);

        memcpy(temp2, buffer, length);

        temp2[length - 1] = '\0';

        buffer += (length - 1);
        requests[i] = parse_request(temp2, &error);

        if (error != NULL) {
            tempStr1 = stringConcatenator("Invalid request received from client - ", clientAddr, -1);
            serverLog(tempStr1, ERROR);
            free(tempStr1);

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

            /* Breaking due to that an error occurred - no request will be executed */
            requests = NULL;
            break;
        }

        free(temp2);
    }

    free(clientAddr);

    return requests;
}

void doLock(int fd, int lock, int lockType) {
    struct flock fileLock;
    char *tempStr1 = NULL, *tempStr2 = NULL;
    char type[15];

    if(lockType == WRITE) {
        sprintf(type, "write");
        fileLock.l_type = F_WRLCK;
    }
    else if (lockType == READ) {
        sprintf(type, "read");
        fileLock.l_type = F_RDLCK;
    }

    if (lock) {
        fileLock.l_start = 0;
        fileLock.l_whence = SEEK_SET;
        fileLock.l_len = 0;
        fileLock.l_pid = getpid();
        
        if(fcntl(fd, F_SETLKW, &fileLock) == -1) {
            tempStr1 = stringConcatenator("Could not acquire ", type, -1);
            tempStr2 = stringConcatenator(tempStr1, " lock for file with fd: ", fd);
            free(tempStr1);
            tempStr1 = stringConcatenator(tempStr2, " - pid: ", getpid());
            serverLog(tempStr1, ERROR);
            free(tempStr1);
            free(tempStr2);
        }
        else {
            tempStr1 = stringConcatenator("Acquired ", type, -1);
            tempStr2 = stringConcatenator(tempStr1, " lock for file with fd: ", fd);
            free(tempStr1);
            tempStr1 = stringConcatenator(tempStr2, " - pid: ", getpid());
            serverLog(tempStr1, INFO);
            free(tempStr1);
            free(tempStr2);
        }
    }
    else {
        fileLock.l_type = F_ULOCK;
        fileLock.l_whence = SEEK_SET;
        fileLock.l_len = 0;
        fileLock.l_pid = getpid();

        if (fcntl(fd, F_SETLK, &fileLock) == -1) {
            tempStr1 = stringConcatenator("Could not release ", type, -1);
            tempStr2 = stringConcatenator(tempStr1, " lock for file with fd: ", fd);
            free(tempStr1);
            tempStr1 = stringConcatenator(tempStr2, " - pid: ", getpid());
            serverLog(tempStr1, ERROR);
            free(tempStr1);
            free(tempStr2);
        }
        else {
            tempStr1 = stringConcatenator("Released ", type, -1);
            tempStr2 = stringConcatenator(tempStr1, " lock for file with fd: ", fd);
            free(tempStr1);
            tempStr1 = stringConcatenator(tempStr2, " - pid: ", getpid());
            serverLog(tempStr1, INFO);
            free(tempStr1);
            free(tempStr2);
        }
    }
}

void addChild(pid_t pid) {
    char *tempStr1 = NULL, *tempStr2 = NULL;

    if (childArray == NULL) {
        childArray = malloc(sizeof(int)*10);
    }

    if (childCtr % 10 == 0) {
        childArray = realloc(childArray, (sizeof(int) * (childCtr + 10)));
    }

    childArray[childCtr++] = pid;

    tempStr1 = stringConcatenator("Added child with pid: ", "", pid);
    tempStr2 = stringConcatenator(tempStr1, " to array - current amount of children: ", childCtr);
    serverLog(tempStr2, INFO);
    free(tempStr1);
    free(tempStr2);
}

void removeChild(pid_t pid){
    char *tempStr1 = NULL, *tempStr2 = NULL;
    if (childArray != NULL) {
        int foundPos = -1;
        for (int i = 0; (i < childCtr) && foundPos == -1; i++) {
            if (childArray[i] == pid) {
                foundPos = i;
            }
        }

        if (foundPos != -1) {
            if (childCtr == 1) {
                childArray[foundPos] = -1;
            }
            else if (childCtr > 1) {
                childArray[foundPos] = childArray[(childCtr - 1)];
                childArray[(childCtr - 1)] = -1;
            }

            childCtr--;
            
            tempStr1 = stringConcatenator("Removed child with pid: ", "", pid);
            tempStr2 = stringConcatenator(tempStr1, " from array - current amount of children: ", childCtr);
            serverLog(tempStr2, INFO);
            free(tempStr1);
            free(tempStr2);
        }
    }
}

void terminateChildren() {
    /* Timer structure */
    struct itimerval cur, prev;

    /* Signal for timer interrupt */
    struct sigaction timerAct;
    sigemptyset(&timerAct.sa_mask);
    timerAct.sa_handler = timerHandler;
    timerAct.sa_flags = 0;
    sigaction(SIGPROF, &timerAct, NULL);

    /* Setting timer parameters */
    cur.it_interval.tv_usec = 0;
    cur.it_interval.tv_sec = 0;
    cur.it_value.tv_usec = 0;
    cur.it_value.tv_sec = (long int) 30;
    setitimer(ITIMER_PROF, &cur, &prev);

    int currentChild = 0;

    /* Sending SIGUSR1 (shutdown signal) to children - one by one */
    for (int i = 0; i < childCtr; i++) {
        currentChild = childArray[i];
        kill(currentChild, SIGUSR1);
    }

    /* Wait until all children have terminated */
    while(childCtr != 0) {
    }

    /* If all children have been terminated - free childArray memory */
    if (childCtr == 0) {
        free(childArray);
        childArray = NULL;
    }
}

void timerHandler() {
    pid_t pid = getpid();
    write(STDOUT_FILENO, "Children took too long to exit - terminating forcefully\n", (strlen("Children took too long to exit - terminating forcefully\n") + 1));
    
    /* Freeing what can be freed - there will be memory leaks */
    free(childArray);
    childArray = NULL;
    
    kill(-pid, SIGKILL);
}

void killChild() {
    int curErr = errno;
    runChildren = 0;
    errno = curErr;
}