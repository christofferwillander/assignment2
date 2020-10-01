#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

<<<<<<< HEAD
#include "request.h"
=======
#include "../include/request.h"
>>>>>>> christoffer


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
    char lineTemp[100];
    fgets(lineTemp, 100, ptr);
    int linesize = strlen(lineTemp);
    fseek(ptr, linesize-1, SEEK_SET);

    while(fgets(output, sizeof(output), ptr))
    {
        for(int i = 0; i<strlen(output); i++)
        {
            if(output[i] != '\'')
            {
                if(output[i] != ':')
                {
                    printf("%c", output[i]);
                }
                else
                {
                    printf("\t");
                } 
            }
        }
    }
    printf("\n");
    fclose(ptr);
}