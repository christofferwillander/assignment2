#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../include/request.h"
extern char databasePath[];
extern void doWriteLock(int fd, int lock);

void insertToFile(FILE *ptr, char *value, int check);
int numberOfColumns(FILE *ptr);
int checkBuffersize(request_t *req, FILE *ptr);

int numberOfColumns(FILE *ptr)
{
    int count = 0;
    char lineTemp[200];
    char *curPos;

    fseek(ptr, 0, SEEK_SET);
    fgets(lineTemp, 200, ptr);

    curPos = lineTemp;

    while(strstr(curPos, ":") != NULL) {
        curPos = strstr(curPos, ":") + 1;
        count++;
    }

    count = count / 2;
    return count;
}

int checkBuffersize(request_t *req, FILE *ptr)
{
    column_t *end;
    column_t *temp = req->columns;
    char *temp1 = NULL, *temp2 = NULL, *temp3 = NULL;
    int len = 0;
    int dig = 0;
    int columns = numberOfColumns(ptr);
    int *sizeArr = malloc(columns);
    int sizecounter = 0;
    int *sizeArrFromFile = malloc(columns);
    int sizeFromFileCounter = 0;
    int retval = 1;
    fseek(ptr, 0, SEEK_SET);
    //find all the sizes in order in the request
    while (temp != NULL)
    {   
        if(temp->data_type == DT_VARCHAR)
        {
            sizeArr[sizecounter++] = strlen(temp->char_val);
        }

        end=temp->next;
        temp = end;    
    }

    temp1 = malloc(100);
    fgets(temp1, 100, ptr);
    temp3 = strstr(temp1, "\n");
    len = (int)(temp3 - temp1) + 1;
    temp2 = malloc(len);
    memcpy(temp2, temp1, len);
    //Find all the max sizes from the table file
    for(int i = 0; i < strlen(temp2); i++)
    {
        if( isdigit(temp2[i]) != 0)
        {
            //printf("dig -> %c\n", temp2[i]);
            sizeArrFromFile[sizeFromFileCounter++] = (temp2[i] - '0') + 2; // +2 compensate for the '' signs
        }
    }

    //compare the sizes found, if the incomming size exceeds the max size it will not add anything to the table
    for(int i = 0; i < sizeFromFileCounter; i++)
    {
        //printf("cmd-> %d\t vs\t org -> %d\n", sizeArrFromFile[i], sizeArr[i]);
        if(sizeArrFromFile[i] < sizeArr[i])
        {
            retval = 0;
        }

    }

    //printf("string2 %s\n", temp2);

    free(sizeArr);
    free(sizeArrFromFile);
    free(temp1);
    free(temp2);
    return retval;

}

void insertToFile(FILE *ptr, char *value, int check)
{
    if(check != 1)
    {
        fprintf(ptr, "%s:", value);
    }
    else
    {
        fprintf(ptr, "%s\n", value);
    }

}

void insertToTable(request_t *req, int clientSocket)
{
    char *intValue;
    char *charValue;
    int columnCount=0;
    char *filePath = malloc(strlen(databasePath) + strlen(req->table_name) + 1);
    strcpy(filePath, databasePath);
    strcat(filePath,req->table_name);
    FILE *file = fopen(filePath, "a+");
    doWriteLock(fileno(file), 1);

    if(access(filePath, F_OK) == -1)
    {
        send(clientSocket, "ERROR: Table does not exist\n", strlen("ERROR: Table does not exist\n"), 0);
    }
    else if (checkBuffersize(req, file) == 0)
    {
       send(clientSocket, "VARCHAR exceeds size\n", sizeof("VARCHAR exceeds size\n"), 0);
    }
    else
    {
        int columnsNr= 0;
        int check = 0;
        columnCount = numberOfColumns(file);

        column_t *end;
        column_t *temp = req->columns;
        fseek(file, 0, SEEK_END);
        while(temp != NULL)
        {
            columnsNr++;
            if(columnsNr == columnCount)
            {
                check = 1;
            }
            if(temp->data_type != 0)
            {
                //data type is varchar
                charValue=temp->char_val;
                insertToFile(file, charValue, check); 

            }
            else
            {
                //data typ is int
                intValue = malloc(sizeof(temp->int_val));
                sprintf(intValue, "%d", temp->int_val);
                insertToFile(file, intValue, check);
                free(intValue);

            }
            end=temp->next;
            temp = end;   
        }
        send(clientSocket, "Successfully added to table\n", sizeof("Successfully added to table\n"), 0);
        
    }
    doWriteLock(fileno(file), 0);
    fclose(file);
    free(filePath);
}
