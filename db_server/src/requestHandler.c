#include "../include/request.h"
#include "createTable.c"
#include "insertToTable.c"
#include "listTables.c"
#include "selectTable.c"
#include "showSchema.c"
extern char databasePath[];
void handleRequest(request_t *request, int clientSocket) {
    switch (request->request_type)
        {
        case RT_CREATE:
            create(request, clientSocket);
            break;
        case RT_SCHEMA:
            getSchema(request, clientSocket);
            break;
        case RT_INSERT:
            insert(request);
            break;
        case RT_SELECT:
            selectTable(request);
            break;
        case RT_TABLES:
            listTables(databasePath, clientSocket);
            break;
        default:
            printf("[-] Unknown request received\n");
            break;
        }
    
    destroy_request(request);
}