#include <stdio.h>
#include <stdlib.h>

#include "request.h"

int main(int argc, char* argv[]) {

    request_t *request;

    if (argc != 2) {
        printf("Provide one request string in quotation marks!\n");
        exit(1);
    }

    request = parse_request(argv[1]);

    print_request(request);

    destroy_request(request);

}
