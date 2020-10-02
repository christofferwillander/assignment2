#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../include/request.h"
extern char databasePath[];

int numberOfColumns(FILE *ptr, char *filename)
{

    int count = 0;
    char lineTemp[200];
    ptr = fopen(filename, "r");
    fgets(lineTemp, 200, ptr);
   // printf("%s", lineTemp);

    const char *del = ":";
    char *templine = strtok(lineTemp, del);
    
    while(templine != NULL)
    {
        
        if(strcmp(templine, "VARCHAR") == 0 || strcmp(templine, "INT") == 0)
        {
            //printf("%s", templine);
            count++;
        }
        templine = strtok(NULL, del);
    }
    //printf("%d",count);
    fclose(ptr);
    return count;

}

void insertToFile(char *filename, FILE *ptr, char *value, int check)
{
    ptr = fopen(filename, "a");
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
                //data type is varchar
                charValue=temp->char_val;
                insertToFile(filePath, file, charValue, check);

            }
            else
            {
                //data typ is int
                sprintf(intValue, "%d", temp->int_val);
                insertToFile(filePath, file, intValue, check);

            }
            end=temp->next;
            temp = end;   
        }
        send(clientSocket, "Successfully added to table\n", strlen("Successfully added to table\n"), 0);
    }
    free(filePath);
}