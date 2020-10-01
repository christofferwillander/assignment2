#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
<<<<<<< HEAD
#include "request.h"

=======
#include "../include/request.h"

extern char databasePath[];
>>>>>>> christoffer

void writeToFile(char *filename, FILE *ptr, char *name, char *dataT, int size, int check)
{
    char charSize[10];
<<<<<<< HEAD
    if(access(filename, F_OK) != -1)
    {
        printf("table already exists!\n");
        exit(1);
    }
=======

>>>>>>> christoffer
    ptr = fopen(filename, "a");

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
    fclose(ptr);
}

<<<<<<< HEAD
void create(request_t *req)
=======
void create(request_t *req, int clientSocket)
>>>>>>> christoffer
{
    char *fileName;
    char *cName;
    char *dataType;
    int size = 0;
    char *openFilePath;
    int check = 0;

<<<<<<< HEAD
    fileName = req->table_name;
    
    char filePath[100] = "../../database/";
    char *totFile = strcat(filePath,fileName);

    FILE *file;

    column_t *end;
    column_t *temp = req->columns;

    while(temp != NULL)
    {
        if(temp->next == NULL)
        {
            check = 1;
        }
        //printf("name : %s\n",temp->name);
        cName=temp->name;
        if(temp->data_type != 0)
        {
            //printf("data type : %d\n",temp->data_type);
            dataType="VARCHAR";
            //printf("char size : %d\n",temp->char_size);
            size=temp->char_size;

        }
        else
        {
            //printf("char value is an INT, no size\n");
            dataType="INT";
        }

        writeToFile(totFile, file, cName, dataType, size, check);

        end=temp->next;
        temp = end;   

    }


=======
    char *filePath = malloc(strlen(databasePath) + strlen(req->table_name) + 1);
    strcpy(filePath, databasePath);
    strcat(filePath,req->table_name);

    if(access(filePath, F_OK) != -1)
    {
        send(clientSocket, "ERROR: Table already exists\n", sizeof("ERROR: Table already exists\n"), 0);
    }
    else {
        FILE *file;

        column_t *end;
        column_t *temp = req->columns;

        while(temp != NULL)
        {
            if(temp->next == NULL)
            {
                check = 1;
            }
            //printf("name : %s\n",temp->name);
            cName=temp->name;
            if(temp->data_type != 0)
            {
                //printf("data type : %d\n",temp->data_type);
                dataType="VARCHAR";
                //printf("char size : %d\n",temp->char_size);
                size=temp->char_size;

            }
            else
            {
                //printf("char value is an INT, no size\n");
                dataType="INT";
            }

            writeToFile(filePath, file, cName, dataType, size, check);

            end=temp->next;
            temp = end;   

        }
        send(clientSocket, "Table was succesfully created\n", sizeof("Table was succesfully created\n"), 0);
    }
    free(filePath);
>>>>>>> christoffer
}

