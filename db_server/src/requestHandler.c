#include "../include/request.h"
#include "createTable.c"
#include "insertToTable.c"
#include "listTables.c"
#include "selectTable.c"
#include "showSchema.c"
#include "dropTable.c"

#define SUCCESS 0
#define ERROR 1
#define INFO 2

extern char databasePath[];
extern char *stringConcatenator(char* str1, char* str2, int num);
extern void serverLog(char* msg, int type);
void handleRequest(request_t *request, int clientSocket) {
    char *tempStr1 = NULL;

    switch (request->request_type)
        {
        case RT_CREATE:
            tempStr1 = stringConcatenator("Create table request received - ", request->table_name, -1);
            serverLog(tempStr1, INFO);
            free(tempStr1);
            createTable(request, clientSocket);
            break;
        case RT_TABLES:
            serverLog("Tables command received", INFO);
            listTables(databasePath, clientSocket);
            break;
        case RT_SCHEMA:
            tempStr1 = stringConcatenator("Schema request received - ", request->table_name, -1);
            serverLog(tempStr1, INFO);
            free(tempStr1);
            getSchema(request, clientSocket);
            break;
        case RT_DROP:
            /* To be implemented */
            tempStr1 = stringConcatenator("Drop request received - ", request->table_name, -1);
            serverLog(tempStr1, INFO);
            drop(request, clientSocket);
            free(tempStr1);
            break;
        case RT_INSERT:
            /* To be adjusted to support socket communications */
            tempStr1 = stringConcatenator("Insert to table request received - ", request->table_name, -1);
            serverLog(tempStr1, INFO);
            free(tempStr1);
            insertToTable(request, clientSocket);
            break;
        case RT_SELECT:
            /* To be adjusted to support socket communications */
            tempStr1 = stringConcatenator("Select from table request received - ", request->table_name, -1);
            serverLog(tempStr1, INFO);
            free(tempStr1);
            selectTable(request, clientSocket);
            break;
        case RT_DELETE:
            /* To be implemented */
            tempStr1 = stringConcatenator("Delete request received - ", request->table_name, -1);
            serverLog(tempStr1, INFO);
            free(tempStr1);
            break;
        case RT_UPDATE:
            /* To be adjusted to support socket communications */
            tempStr1 = stringConcatenator("Update request received - ", request->table_name, -1);
            serverLog(tempStr1, INFO);
            free(tempStr1);
            break;
        default:
            /* This should never happen - something is really wrong if we end up here */
            printf("[-] Unknown request received\n");
            break;
        }
    
    destroy_request(request);
}
