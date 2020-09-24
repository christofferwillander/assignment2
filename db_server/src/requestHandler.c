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
            printf("[+] Create table request received (%s)\n", request->table_name);
            create(request, clientSocket);
            break;
        case RT_TABLES:
            printf("[+] Tables command received\n");
            listTables(databasePath, clientSocket);
            break;
        case RT_SCHEMA:
            printf("[+] Schema request received (%s)\n", request->table_name);
            getSchema(request, clientSocket);
            break;
        case RT_DROP:
            printf("[+] Drop request received (%s)\n", request->table_name);
            break;
        case RT_INSERT:
            printf("[+] Insert to table request received (%s)\n", request->table_name);
            insert(request);
            break;
        case RT_SELECT:
            printf("[+] Select from table request received (%s)\n", request->table_name);
            selectTable(request);
            break;
        case RT_DELETE:
            printf("[+] Delete request received (%s)\n", request->table_name);
            break;
        case RT_UPDATE:
            printf("[+] Update request received (%s)\n", request->table_name);
            break;
        default:
            printf("[-] Unknown request received\n");
            break;
        }
    
    destroy_request(request);
}