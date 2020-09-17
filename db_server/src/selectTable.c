#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "request.h"


void selectTable(request_t *req)
{
    char *filename = req->table_name;
    char filePath[100] = "../../database/";
    char *totFile = strcat(filePath,filename);
    char output[256];
    //char *fileout;
    if(access(totFile, F_OK) == -1)
    {
        printf("table does not exists!\n");
        exit(1);
    }

    FILE *ptr;
    ptr = fopen(totFile, "r");


    while(fgets(output, sizeof(output), ptr))
    {
        printf("%s", output);
    }
    fclose(ptr);
}