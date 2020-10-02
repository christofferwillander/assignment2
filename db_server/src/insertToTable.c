#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../include/request.h"

int numberOfColumns(FILE *ptr, char *filename)
{
    if(access(filename, F_OK) == -1)
    {
        printf("table does not exist\n");
        //exit(1);
    }
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
    char charSize[10];
    ptr = fopen(filename, "a");
    if(check != 1)
    {
        fputs(value,ptr);
        fputs(":", ptr);
    }
    else
    {
        fputs(value,ptr);    
        fputs("\n",ptr);
    }
    
    fclose(ptr);
}

void insert(request_t *req)
{
    char *fileName;
    char *intValue;
    char *charValue;
    int columnCount=0;
    fileName = req->table_name;
    char filePath[100] = "../../database/";
    char *totFile = strcat(filePath,fileName);
    
    FILE *file;
    FILE *linePtr;
    int columnsNr= 0;
    int check = 0;
    columnCount = numberOfColumns(linePtr, totFile);

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
            insertToFile(totFile, file, charValue, check);

        }
        else
        {
            //data typ is int
            sprintf(intValue, "%d", temp->int_val);
            insertToFile(totFile, file, intValue, check);

        }

        end=temp->next;
        temp = end;   

    }


}

