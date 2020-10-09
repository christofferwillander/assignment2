#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../include/request.h"

#define UNLOCK 0
#define LOCK 1
#define WRITE 0
#define READ 1

extern void doLock(int fd, int lock, int lockType);
extern char databasePath[];

void drop(request_t *req, int clientSocket)
{
    char *filePath = malloc(strlen(databasePath) + strlen(req->table_name) + 1);
    strcpy(filePath, databasePath);
    strcat(filePath,req->table_name);

    FILE *file = fopen(filePath, "w");
    doLock(fileno(file), LOCK, WRITE);


    if(access(filePath, F_OK) == -1)
    {
        send(clientSocket, "ERROR: Table does not exist\n", sizeof("ERROR: Table does not exist\n"), 0);
    }
    else 
    {
        int del = remove(filePath);
        if (!del)
        {
            send(clientSocket, "The table was removed successfully\n", strlen("The table was removed successfully\n"), 0 );
        }
        else
        {
            send(clientSocket, "The table could not be removed\n", strlen("The table could not be removed\n"), 0 );
        }
    }
    
    fclose(file);
    free(filePath);
}