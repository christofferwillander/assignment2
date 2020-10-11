#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>

#define SUCCESS 0
#define ERROR 1
#define INFO 2
#define AUDIT 3

extern char databasePath[];
extern void serverLog(char *msg, int type);

void listTables(int clientSocket) {
    /* Directory pointer, directory entry pointer */
    DIR *directory = opendir(databasePath);
    struct dirent *directoryEntry;

    /* If database directory could not be opened... */
    if (directory == NULL) {
        serverLog("ERROR: Could not open database directory", ERROR);
    } /* Else, if directory was successfully opened... */
    else {
        /* While there are files left to iterate through in database directory... */
        while((directoryEntry = readdir(directory)) != NULL) {
            /* If file is not a hidden file, print name (i.e. table name) */
            if (directoryEntry->d_name[0] != '.') {
                send(clientSocket, (directoryEntry->d_name), strlen((directoryEntry->d_name))*sizeof(char) + 1, 0);
                send(clientSocket, "\n", sizeof("\n"), 0);
            }
        }
        closedir(directory);
    }
}