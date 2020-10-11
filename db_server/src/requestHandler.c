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
#define AUDIT 3

extern char databasePath[];
extern char *stringConcatenator(char* str1, char* str2, int num);
extern void serverLog(char* msg, int type);

void handleRequest(request_t *request, int clientSocket, char* client) {
    char *tempStr1 = NULL, *tempStr2 = NULL;
    int statusCode = -1;

    switch (request->request_type)
        {
        case RT_CREATE:
            tempStr1 = stringConcatenator("CREATE TABLE request received - ", request->table_name, -1);
            serverLog(tempStr1, INFO);
            free(tempStr1);
            statusCode = createTable(request, clientSocket);

            if (statusCode == SUCCESS) {
                tempStr1 = stringConcatenator("Succesfully created table ", request->table_name, -1);
                tempStr2 = stringConcatenator(tempStr1, " - ", -1);
                free(tempStr1);
                tempStr1 = stringConcatenator(tempStr2, client, -1);
                serverLog(tempStr1, AUDIT);
                free(tempStr1);
                free(tempStr2);
            }
            else if (statusCode == ERROR) {
                tempStr1 = stringConcatenator("Failed to create table ", request->table_name, -1);
                tempStr2 = stringConcatenator(tempStr1, " - ", -1);
                free(tempStr1);
                tempStr1 = stringConcatenator(tempStr2, client, -1);
                serverLog(tempStr1, AUDIT);
                free(tempStr1);
                free(tempStr2);
            }

            break;
        case RT_TABLES:
            serverLog("Table listing request received", INFO);
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
            free(tempStr1);
            statusCode = drop(request, clientSocket);

            if (statusCode == SUCCESS) {
                tempStr1 = stringConcatenator("Succesfully dropped table ", request->table_name, -1);
                tempStr2 = stringConcatenator(tempStr1, " - ", -1);
                free(tempStr1);
                tempStr1 = stringConcatenator(tempStr2, client, -1);
                serverLog(tempStr1, AUDIT);
                free(tempStr1);
                free(tempStr2);
            }
            else if (statusCode == ERROR) {
                tempStr1 = stringConcatenator("Failed to drop table ", request->table_name, -1);
                tempStr2 = stringConcatenator(tempStr1, " - ", -1);
                free(tempStr1);
                tempStr1 = stringConcatenator(tempStr2, client, -1);
                serverLog(tempStr1, AUDIT);
                free(tempStr1);
                free(tempStr2);
            }

            break;
        case RT_INSERT:
            tempStr1 = stringConcatenator("INSERT to table request received - ", request->table_name, -1);
            serverLog(tempStr1, INFO);
            free(tempStr1);
            statusCode = insertToTable(request, clientSocket);

            if (statusCode == SUCCESS) {
                tempStr1 = stringConcatenator("Succesfully inserted new row into table ", request->table_name, -1);
                tempStr2 = stringConcatenator(tempStr1, " - ", -1);
                free(tempStr1);
                tempStr1 = stringConcatenator(tempStr2, client, -1);
                serverLog(tempStr1, AUDIT);
                free(tempStr1);
                free(tempStr2);
            }
            else if (statusCode == ERROR) {
                tempStr1 = stringConcatenator("Failed to insert new row into table ", request->table_name, -1);
                tempStr2 = stringConcatenator(tempStr1, " - ", -1);
                free(tempStr1);
                tempStr1 = stringConcatenator(tempStr2, client, -1);
                serverLog(tempStr1, AUDIT);
                free(tempStr1);
                free(tempStr2);
            }

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
