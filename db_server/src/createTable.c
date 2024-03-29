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

void writeToFile(FILE *ptr, char *name, char *dataT, int size, int isEOL)
{
    char charSize[10];

    fputs(name, ptr);
    fputs(":", ptr);
    fputs(dataT, ptr);
    fputs(":", ptr);
    if(size != 0)
    {
        sprintf(charSize,"%d", size);
        fputs(charSize, ptr);
        fputs(":", ptr);
    }
    if(isEOL == 1)
    {
        fputs("\n",ptr);
    }
}

int createTable(request_t *req, int clientSocket)
{
    char *fileName;
    char *cName;
    char *dataType;
    int size = 0;
    char *openFilePath;
    int isEOL = 0;
    int success = ERROR;

    char *filePath = malloc(strlen(databasePath) + strlen(req->table_name) + 1);
    strcpy(filePath, databasePath);
    strcat(filePath,req->table_name);

    if(access(filePath, F_OK) != -1)
    {
        send(clientSocket, "ERROR: Table already exists\n", sizeof("ERROR: Table already exists\n"), 0);
    }
    else 
    {
        FILE *file = fopen(filePath, "w+");
        doLock(fileno(file), LOCK, WRITE);
        column_t *end;
        column_t *temp = req->columns;

        while(temp != NULL)
        {
            if(temp->next == NULL)
            {
                isEOL = 1;
            }
            cName=temp->name;
            if(temp->data_type != 0)
            {
                dataType="VARCHAR";
                size=temp->char_size;

            }
            else
            {
                size = 0;
                dataType="INT";
            }

            writeToFile(file, cName, dataType, size, isEOL);

            end=temp->next;
            temp = end;   

        }
        doLock(fileno(file), UNLOCK, WRITE);
        fclose(file);
        send(clientSocket, "Table was succesfully created\n", sizeof("Table was succesfully created\n"), 0);
        success = SUCCESS;
    }
    free(filePath);

    return success;
}
