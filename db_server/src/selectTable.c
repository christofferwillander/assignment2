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

void selectTable(request_t *req, int clientSocket)
{
    char *filePath = malloc(strlen(databasePath) + strlen(req->table_name) + 1);
    strcpy(filePath, databasePath);
    strcat(filePath,req->table_name);

    if(access(filePath, F_OK) == -1)
    {
        send(clientSocket, "ERROR: Table does not exist\n", strlen("ERROR: Table does not exist\n"), 0);
    }
    else
    {
        FILE *ptr = fopen(filePath, "r");
        FILE *columns;
        int colNr = numberOfColumns(ptr);
        int checkCol = 0;
        fseek(ptr, 0, SEEK_SET);
        char getString[256];
        
        doLock(fileno(ptr), LOCK, READ);

        //skip the first line
        char lineTemp[100];
        fgets(lineTemp, 100, ptr);
        // start read on next line
        int linesize = strlen(lineTemp);
        fseek(ptr, linesize, SEEK_SET);
        const char *del = ":";

        while (fgets(getString, sizeof(getString), ptr) != NULL)
        {
            char *templine = strtok(getString, del);
            while (templine != NULL)
            {
                checkCol++;
                if(checkCol != colNr)
                {
                    send(clientSocket, templine, strlen(templine), 0);
                    send(clientSocket, "\t", strlen("\t"), 0);
                }
                else
                {
                    send(clientSocket, templine, strlen(templine), 0);
                    checkCol = 0;
                }
                templine = strtok(NULL, del);
            }
        }
        send(clientSocket, "\n", strlen("\n"), 0);
        doLock(fileno(ptr), UNLOCK, READ);
        fclose(ptr);
    }

    free(filePath);
}