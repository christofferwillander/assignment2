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
int validateInput(request_t *reqest, FILE *ptr, int clientSocket);

int numberOfColumns(FILE *ptr)
{
    int count = 0;

    char lineTemp[200];
    char *curPos;

    fseek(ptr, 0, SEEK_SET);
    fgets(lineTemp, 200, ptr);

    curPos = lineTemp;

    while(strstr(curPos, "INT") != NULL) {
        curPos = strstr(curPos, "INT") + strlen("INT");
        count++;
    }

    curPos = lineTemp;

    while(strstr(curPos, "VARCHAR") != NULL) {
        curPos = strstr(curPos, "VARCHAR") + strlen("VARCHAR");
        count++;
    }

    return count;
}

int validateInput(request_t *request, FILE *ptr, int clientSocket) {
    int nrOfCols = numberOfColumns(ptr);
    int statusCode = 0;
    int inputCounter = 0, tableCounter = 0; 

    column_t *curCol = request->columns;
    int *inputTypes = NULL, *inputSizes = NULL;
    int *tableTypes = NULL, *tableSizes = NULL;

    /* Allocating memory */
    if (nrOfCols > 0) {
        inputTypes = malloc(sizeof(int) * nrOfCols);
        inputSizes = malloc(sizeof(int) * nrOfCols);
        tableTypes = malloc(sizeof(int) * nrOfCols);
        tableSizes = malloc(sizeof(int) * nrOfCols);
    }

    /* Fetching data types and sizes from incoming request */
    while (curCol != NULL) {
        if (curCol->data_type == DT_INT) {
            inputTypes[inputCounter] = DT_INT;
            inputSizes[inputCounter++] = -1;
        }
        else if (curCol->data_type == DT_VARCHAR) {
            inputTypes[inputCounter] = DT_VARCHAR;
            inputSizes[inputCounter++] = strlen(curCol->char_val);
        }

        curCol = curCol->next;
    }

    /* Setting file pointer to beginning of file */
    fseek(ptr, 0, SEEK_SET);
    char *tempLine = malloc(sizeof(char) * 200);

    /* Reading database file header */
    fgets(tempLine, 200, ptr);

    char *strPtr1 = NULL, *strPtr2 = NULL, *tempStr = NULL;
    int strLength = 0, retrieveSize = 0;

    strPtr1 = tempLine;

    /* Determining data types and sizes based on database file header */
    while (strstr(strPtr1, ":") != NULL) {
        strPtr2 = strPtr1;
        strPtr1 = strstr(strPtr1, ":");
        strLength = (strPtr1 - strPtr2);

        if (strLength > 0) {
            tempStr = malloc((sizeof(char) * strLength) + 1);
            memcpy(tempStr, strPtr2, strLength);
            tempStr[strLength] = '\0';
            strPtr1 += 1;

            if (strcmp("INT", tempStr) == 0) {
                tableSizes[tableCounter] = -1;
                tableTypes[tableCounter++] = DT_INT;
            }
            else if (strcmp("VARCHAR", tempStr) == 0)
            {
                tableTypes[tableCounter] = DT_VARCHAR;
                retrieveSize = 1;    
            }
            else if (retrieveSize == 1) {
                tableSizes[tableCounter++] = atoi(tempStr);
                retrieveSize = 0;
            }

            /* Freeing memory for temporary string */
            free(tempStr);
        }
    }

    /* Freeing allocated memory for tempLine */
    free(tempLine);

    /* Ensuring amount of input columns correspond to the amount in the table */
    if (tableCounter != inputCounter) {
        statusCode = 1;
        send(clientSocket, "ERROR: Incorrect amount of columns in provided INSERT statement\n", sizeof("ERROR: Incorrect amount of columns in provided INSERT statement\n"), 0);
    }

    /* Checking that all data types match */
    for (int i = 0; (i < nrOfCols) && statusCode != 1; i++) {
        if (inputTypes[i] != tableTypes[i]) {
            statusCode = 1;
            send(clientSocket, "ERROR: Incorrect data types in provided INSERT statement\n", sizeof("ERROR: Incorrect data types in provided INSERT statement\n"), 0);
        }
    }

    /* Checking that sizes are in correct range */
    for (int i = 0; (i < nrOfCols) && statusCode != 1; i++) {
        if ((inputSizes[i] > tableSizes[i])) {
            statusCode = 1;
            send(clientSocket, "ERROR: Incorrect data sizes in provided INSERT statement\n", sizeof("ERROR: Incorrect data sizes in provided INSERT statement\n"), 0);
        }
    }

    /* Freeing allocated memory */
    if (tableTypes != NULL && tableSizes != NULL && inputTypes != NULL && inputSizes != NULL) {
        free(inputTypes);
        free(inputSizes); 
        free(tableTypes);
        free(tableSizes);
    }
    
    return statusCode;
}

void insertToFile(FILE *ptr, char *value, int isEOL)
{
    if(isEOL != 1)
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
    FILE *file;

    if(access(filePath, F_OK) == -1)
    {
        send(clientSocket, "ERROR: Table does not exist\n", sizeof("ERROR: Table does not exist\n"), 0);
    }
    else {
        file = fopen(filePath, "a+");
        doLock(fileno(file), LOCK, WRITE);
        if (validateInput(req, file, clientSocket) == 1) {
            /* No bueno */
        }
        else
        {
            int columnsNr= 0;
            int isEOL = 0;
            columnCount = numberOfColumns(file);

            column_t *end;
            column_t *temp = req->columns;
            fseek(file, 0, SEEK_END);
            while(temp != NULL)
            {
                columnsNr++;
                if(columnsNr == columnCount)
                {
                    isEOL = 1;
                }

                if(temp->data_type != 0)
                {
                    //data type is varchar
                    charValue=temp->char_val;
                    insertToFile(file, charValue, isEOL); 

                }
                else
                {
                    //data typ is int
                    intValue = malloc(sizeof(temp->int_val));
                    sprintf(intValue, "%d", temp->int_val);
                    insertToFile(file, intValue, isEOL);
                    free(intValue);

                }
                end=temp->next;
                temp = end;   
            }
            send(clientSocket, "Successfully added to table\n", sizeof("Successfully added to table\n"), 0);
            
        }
        doLock(fileno(file), UNLOCK, WRITE);
        fclose(file);
    }

    free(filePath);
}
