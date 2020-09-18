#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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
}