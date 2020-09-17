#include <stdio.h>
#include <stdlib.h>
#include "../include/request.h"
#include "listTables.c"
#include "server.c"

int main(int argc, char* argv[]) {
    char *databasePath = "../../database/";
    request_t *request;

    char *receivedRequest = serve(0);
    printf("Sending the following query to SQL parser: %s\n\n", receivedRequest);

    request = parse_request(receivedRequest);

    printf("Parsed SQL query: \n");
    print_request(request);

    free(receivedRequest);
    destroy_request(request); 
}