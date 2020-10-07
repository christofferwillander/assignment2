#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "request.h"
extern char databasePath[];
extern void doWriteLock(int fd, int lock);

void insertToFile(char *filename, FILE *ptr, char *value, int check);
int numberOfColumns(FILE *ptr, char *filename);
int checkBuffersize(request_t *req, char *filename);

int numberOfColumns(FILE *ptr, char *filename)
{

    int count = 0, len = 0;
    char lineTemp[200];
    ptr = fopen(filename, "r");
    char *temp1 = NULL, *temp2 = NULL, *temp3 = NULL;
    

    fgets(lineTemp, 200, ptr);
    //printf("line %s", lineTemp);
 
    const char *del = ":";
    char *templine = strtok(lineTemp, del);
    
    while(templine != NULL)
    {
        
        if(strcmp(templine, "VARCHAR") == 0 || strcmp(templine, "INT") == 0)
        {
            //printf("temp %s", templine);
            count++;
        }
        templine = strtok(NULL, del);
    }
    //printf("count %d",count);
    fclose(ptr);

    return count;

}

int checkBuffersize(request_t *req, char *filename)
{
    //printf("string2\n");
    column_t *end;
    column_t *temp = req->columns;
    FILE *ptr = fopen(filename, "r");
    FILE *filetemp;
    char *temp1 = NULL, *temp2 = NULL, *temp3 = NULL;
    int len = 0;
    int dig = 0;
    int columns = numberOfColumns(filetemp, filename);
    int *sizeArr = malloc(columns);
    int sizecounter = 0;
    int *sizeArrFromFile = malloc(columns);
    int sizeFromFileCounter = 0;
    int retval = 1;

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
    fclose(ptr);
    return retval;

}

void insertToFile(char *filename, FILE *ptr, char *value, int check)
{

    ptr = fopen(filename, "a");
    doWriteLock(fileno(ptr), 1);
    if(check != 1)
    {
        //fputs(value,ptr);
        //fputs(":", ptr);
        fprintf(ptr, "%s:", value);
    }
    else
    {
        //fputs(value,ptr);    
        //fputs("\n",ptr);
        fprintf(ptr, "%s\n", value);
    }
    
    doWriteLock(fileno(ptr), 0);
    fclose(ptr);
}

void insertToTable(request_t *req, int clientSocket)
{
    char *intValue;
    char *charValue;
    int columnCount=0;
    char *filePath = malloc(strlen(databasePath) + strlen(req->table_name) + 1);
    strcpy(filePath, databasePath);
    strcat(filePath,req->table_name);

    if(access(filePath, F_OK) == -1)
    {
        send(clientSocket, "ERROR: Table does not exist\n", strlen("ERROR: Table does not exist\n"), 0);
    }
    else if (checkBuffersize(req, filePath) == 0)
    {
       send(clientSocket, "VARCHAR exceeds size\n", sizeof("VARCHAR exceeds size\n"), 0);
    }
    else
    {
        FILE *file;
        FILE *linePtr;
        int columnsNr= 0;
        int check = 0;
        columnCount = numberOfColumns(linePtr, filePath);

        column_t *end;
        column_t *temp = req->columns;

        while(temp != NULL)
        {
            columnsNr++;
            if(columnsNr == columnCount)
            {
                check = 1;
            }
            if(temp->data_type != 0)
            {
                //printf("columns %d\n", columnCount);
                //data type is varchar
                charValue=temp->char_val;
                insertToFile(filePath, file, charValue, check); 

            }
            else
            {
                //data typ is int
                intValue = malloc(sizeof(temp->int_val));
                sprintf(intValue, "%d", temp->int_val);
                insertToFile(filePath, file, intValue, check);
                free(intValue);

            }
            end=temp->next;
            temp = end;   
        }
        send(clientSocket, "Successfully added to table\n", sizeof("Successfully added to table\n"), 0);
        
    }
    free(filePath);
}
