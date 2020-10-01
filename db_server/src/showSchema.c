#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

<<<<<<< HEAD
#include "request.h"

void getSchema(request_t *req)
{

    char *fileName = req->table_name;
    
    char filePath[100] = "../../database/";
    char *totFile = strcat(filePath,fileName);

    if(access(totFile, F_OK) == -1)
    {
        printf("table does not exist\n");
        exit(1);
    }

    FILE *ptr;
    ptr = fopen(totFile, "r");

    int count = 1;
    char lineTemp[200];
    fgets(lineTemp, 200, ptr);
   // printf("%s", lineTemp);

    const char *del = ":";
    char *templine = strtok(lineTemp, del);
    
    while(templine != NULL)
    {
        if(strcmp(templine, "VARCHAR") == 0)
        {
            templine = strtok(NULL, del);
            printf("VARCHAR(%s)\n", templine);
            count++;
        }
        else if (strcmp(templine, "INT")==0)
        {
            printf("%s\n", templine);
        }
        else
        {
            printf("%s\t", templine);
        }
        
        templine = strtok(NULL, del);

    }
    //printf("%d",count);
    fclose(ptr);
=======
#include "../include/request.h"

extern char databasePath[];

void getSchema(request_t *req, int clientSocket)
{ 
    char *filePath = malloc(strlen(databasePath) + strlen(req->table_name) + 1);
    strcpy(filePath, databasePath);
    strcat(filePath,req->table_name);

    if(access(filePath, F_OK) == -1)
    {
        send(clientSocket, "ERROR: Table does not exist\n", sizeof("ERROR: Table does not exist\n"), 0);
    }
    else {
        FILE *ptr;
        ptr = fopen(filePath, "r");

        char lineTemp[200];
        fgets(lineTemp, 200, ptr);

        const char *del = ":";
        char *templine = strtok(lineTemp, del);
        
        while(templine != NULL)
        {
            if(strcmp(templine, "VARCHAR") == 0)
            {
                templine = strtok(NULL, del);
                send(clientSocket, "VARCHAR(", sizeof("VARCHAR("), 0);
                send(clientSocket, templine, strlen(templine), 0);
                send(clientSocket, ")\n", sizeof(")\n"), 0);
            }
            else if (strcmp(templine, "INT")==0)
            {
                send(clientSocket, templine, strlen(templine), 0);
                send(clientSocket, "\n", sizeof("\n"), 0);
            }
            else
            {
                send(clientSocket, templine, strlen(templine), 0);
                send(clientSocket, "\t", sizeof("\t"), 0);
            }
            
            templine = strtok(NULL, del);
        }

        send(clientSocket, "\n", sizeof("\n"), 0);
        fclose(ptr);
        free(filePath);
    }
>>>>>>> christoffer
}