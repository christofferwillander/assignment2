#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../include/request.h"

#define SUCCESS 0
#define ERROR 1
#define INFO 2
#define AUDIT 3

#define UNLOCK 0
#define LOCK 1
#define WRITE 0
#define READ 1

extern void doLock(int fd, int lock, int lockType);
extern char databasePath[];

int drop(request_t *req, int clientSocket)
{
    int success = ERROR;
    char *filePath = malloc(strlen(databasePath) + strlen(req->table_name) + 1);
    strcpy(filePath, databasePath);
    strcat(filePath,req->table_name);

    FILE *file;

    if(access(filePath, F_OK) == -1)
    {
        send(clientSocket, "ERROR: Table does not exist\n", sizeof("ERROR: Table does not exist\n"), 0);
    }
    else 
    {
        file = fopen(filePath, "w");
        doLock(fileno(file), LOCK, WRITE);

        int del = remove(filePath);
        if (!del)
        {
            success = SUCCESS;
            send(clientSocket, "The table was removed successfully\n", sizeof("The table was removed successfully\n"), 0 );
        }
        else
        {
            send(clientSocket, "The table could not be removed\n", sizeof("The table could not be removed\n"), 0 );
        }
        
        fclose(file);
    }

    free(filePath);

    return success;
}