#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>

extern char databasePath[];
extern void serverLog(char *msg, int type);

void listTables(int clientSocket) {
    /* Directory pointer, directory entry pointer */
    DIR *directory = opendir(databasePath);
    struct dirent *directoryEntry;

    /* If database directory could not be opened... */
    if (directory == NULL) {
        printf("[-] ERROR: Could not open database directory\n");
    } /* Else, if directory was successfully opened... */
    else {
        /* While there are files left to iterate through in database directory... */
        while((directoryEntry = readdir(directory)) != NULL) {
            /* If file is not a hidden file, print name (i.e. table name) */
            if (directoryEntry->d_name[0] != '.') {

                send(clientSocket, (directoryEntry->d_name), strlen((directoryEntry->d_name))*sizeof(char), 0);
                send(clientSocket, "\n", sizeof(char), 0);
            }
        }
        closedir(directory);
    }
}