#include <stdio.h>
#include <stdlib.h>

#include "request.h"


void create(request_t *req)
{
    char *fileName;
    char *name;
    char *dataType;
    int size;

    printf("hellu, create table: %s\n", req->table_name);
    fileName = req->table_name;
    
    column_t *end;
    column_t *temp = req->columns;

    while(temp != NULL)
    {
        printf("name : %s\n",temp->name);
        if(temp->data_type != 0)
        {
            printf("data type : %d\n",temp->data_type);
            printf("char size : %d\n",temp->char_size);

        }
        else
        {
            printf("char value is an INT, no size\n");
        }

        end=temp->next;
        temp = end;   

    }


}