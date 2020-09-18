#include <stdio.h>
#include <stdlib.h>
#include "../include/request.h"
#include "listTables.c"
#include "server.c"
 
int main(int argc, char* argv[]) {
    char *databasePath = "../../database/";
    serve(0);
    
    /*
    request = parse_request(receivedRequest);

    printf("Parsed SQL query: \n");
    print_request(request);

    free(receivedRequest); 
    destroy_request(request); */
}