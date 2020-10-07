#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../include/request.h"

extern char databasePath[];
extern void doWriteLock(int fd, int lock);

void writeToFile(FILE *ptr, char *name, char *dataT, int size, int check)
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
    if(check == 1)
    {
        fputs("\n",ptr);
    }
}

void createTable(request_t *req, int clientSocket)
{
    char *fileName;
    char *cName;
    char *dataType;
    int size = 0;
    char *openFilePath;
    int check = 0;

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
        doWriteLock(fileno(file), 1);
        column_t *end;
        column_t *temp = req->columns;

        while(temp != NULL)
        {
            if(temp->next == NULL)
            {
                check = 1;
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

            writeToFile(file, cName, dataType, size, check);

            end=temp->next;
            temp = end;   

        }
        doWriteLock(fileno(file), 0);
        fclose(file);
        send(clientSocket, "Table was succesfully created\n", sizeof("Table was succesfully created\n"), 0);
    }
    free(filePath);
}
