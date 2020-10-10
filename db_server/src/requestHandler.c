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
            tempStr1 = stringConcatenator("CREATE TABLE request received - ", request->table_name, -1);
            serverLog(tempStr1, INFO);
            free(tempStr1);
            createTable(request, clientSocket);
            break;
        case RT_TABLES:
            serverLog("Table listing reqeust received", INFO);
            listTables(clientSocket);
            break;
        case RT_SCHEMA:
            tempStr1 = stringConcatenator("Schema request received - ", request->table_name, -1);
            serverLog(tempStr1, INFO);
            free(tempStr1);
            getSchema(request, clientSocket);
            break;
        case RT_DROP:
            tempStr1 = stringConcatenator("DROP TABLE request received - ", request->table_name, -1);
            serverLog(tempStr1, INFO);
            drop(request, clientSocket);
            free(tempStr1);
            break;
        case RT_INSERT:
            tempStr1 = stringConcatenator("INSERT to table request received - ", request->table_name, -1);
            serverLog(tempStr1, INFO);
            free(tempStr1);
            insertToTable(request, clientSocket);
            break;
        case RT_SELECT:
            tempStr1 = stringConcatenator("SELECT FROM table request received - ", request->table_name, -1);
            serverLog(tempStr1, INFO);
            free(tempStr1);
            selectTable(request, clientSocket);
            break;
        case RT_DELETE:
            /* To be implemented */
            send(clientSocket, "DELETE command is not yet implemented.\n", sizeof("DELETE command is not yet implemented.\n"), 0);
            tempStr1 = stringConcatenator("DELETE request received - ", request->table_name, -1);
            serverLog(tempStr1, INFO);
            free(tempStr1);
            break;
        case RT_UPDATE:
            /* To be implemented */
            send(clientSocket, "UPDATE command is not yet implemented.\n", sizeof("UPDATE command is not yet implemented.\n"), 0);
            tempStr1 = stringConcatenator("UPDATE request received - ", request->table_name, -1);
            serverLog(tempStr1, INFO);
            free(tempStr1);
            break;
        default:
            /* This should never happen - something is really wrong if we end up here */
            send(clientSocket, "Something went wrong.\n", sizeof("Something went wrong.\n"), 0);
            serverLog("Unknown request received", ERROR);
            break;
        }
        
    destroy_request(request);
}
