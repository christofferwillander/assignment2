#include <stdio.h>
#include <stdlib.h>

#include "createTable.c"
#include "insertToTable.c"
#include "selectTable.c"
#include "showSchema.c"
#include "request.h"

int main(int argc, char* argv[]) {

    request_t *request;
    

    if (argc != 2) {
        printf("Provide one request string in quotation marks!\n");
        exit(1);
    }

    request = parse_request(argv[1]);
    switch (request->request_type)
    {
    case 0: //Create table
        create(request);
        break;
    case 2:
        getSchema(request);
        break;
    case 4: //Insert into table
        insert(request);
        break;
    case 5:
        selectTable(request);
        break;
    
    default:
        printf("Unknown command");
        break;
    }

    destroy_request(request);
}