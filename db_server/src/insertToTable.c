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
    int retval = -1 ;
    fseek(ptr, 0, SEEK_SET);
    //find all the sizes in order in the request
    while (temp != NULL)
    {   
        if(temp->data_type == DT_VARCHAR)
        {
            printf("VARCHAR (%d)\n", sizecounter);
            sizeArr[sizecounter++] = strlen(temp->char_val);
        }
        else if (temp->data_type == DT_INT) {
            printf("INT (%d)\n", sizecounter);
            sizeArr[sizecounter++] = -1;
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

    char *strPtr1 = temp2, *strPtr2 = NULL;

    int strLength = 0;
    int ctr = 0;
    int retreiveNum = 0;
    char *tempStr;

    while (strstr(strPtr1, ":") != NULL) {
        strPtr2 = strPtr1;
        strPtr1 = strstr(strPtr1, ":");
        strLength = (strPtr1 - strPtr2);
        if (strLength > 0 && strPtr1 != NULL) {
            tempStr = malloc(strLength + 1);
            memcpy(tempStr, strPtr2, strLength);
            tempStr[strLength] = '\0';
            strPtr1 += 1;

            if (strcmp("INT", tempStr) == 0) {
                ctr++;
            }
            else if (strcmp("VARCHAR", tempStr) == 0) {
                ctr++;
                retreiveNum = 1;
            }
            else if (retreiveNum == 1 && ctr > 0) {
                sizeArrFromFile[(ctr - 1)] = atoi(tempStr);
                printf("VARCHAR SIZE %d DETECTED (index %d) \n", sizeArrFromFile[ctr - 1], ctr-1);
                retreiveNum = 0;
            }

            free(tempStr);
        }
    }
    //compare the sizes found, if the incomming size exceeds the max size it will not add anything to the table
    for(int i = 0; (i < sizecounter) && (retval != 1); i++)
    {
        //printf("cmd-> %d\t vs\t org -> %d\n", sizeArrFromFile[i], sizeArr[i]);
        if((sizeArr[i] <= sizeArrFromFile[i]) && (sizeArr[i] != -1))
        {
            printf("Correct varchar size (index %d)\n", i);
            retval = 0;
        }
        else if (sizeArr[i] != -1) {
            retval = 1;
            printf("fuck\n");
        }
    }

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
    int columnCount = 0;
    char *filePath = malloc(strlen(databasePath) + strlen(req->table_name) + 1);
    strcpy(filePath, databasePath);
    strcat(filePath,req->table_name);
    FILE *file = fopen(filePath, "a+");
    doLock(fileno(file), LOCK, WRITE);

    if(access(filePath, F_OK) == -1)
    {
        send(clientSocket, "ERROR: Table does not exist\n", strlen("ERROR: Table does not exist\n"), 0);
    }
    else if (checkBuffersize(req, file) == 1)
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
    doLock(fileno(file), UNLOCK, WRITE);
    fclose(file);
    free(filePath);
}
