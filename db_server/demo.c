#include <stdio.h>
#include <stdlib.h>

#include "src/createTable.c"
#include "request.h"

int main(int argc, char* argv[]) {

    request_t *request;
    column_t *column;
    char *name;

    if (argc != 2) {
        printf("Provide one request string in quotation marks!\n");
        exit(1);
    }

    request = parse_request(argv[1]);
   if(request->request_type == 0)
    {
        create(request);
    }



    print_request(request);

    destroy_request(request);

}
