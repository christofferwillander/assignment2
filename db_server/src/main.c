#include <stdio.h>
#include <stdlib.h>

#include "createTable.c"
#include "insertToTable.c"
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
    case 4: //Insert into table
        insert(request);
        break;
    
    default:
        break;
    }

   // if(request->request_type == 0)
  //  {
  //      create(request);
  //  }

    //print_request(request);
    destroy_request(request);
}