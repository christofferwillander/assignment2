#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>


void listTables(char* databasePath) {

    /* Directory pointer, directory entry pointer */
    DIR *directory = opendir(databasePath);
    struct dirent *directoryEntry;

    /* If database directory could not be opened... */
    if (directory == NULL) {
        printf("ERROR: Could not open database directory");
    } /* Else, if directory was successfully opened... */
    else {
        /* While there are files left to iterate through in database directory... */
        while((directoryEntry = readdir(directory)) != NULL) {
            /* If file is not a hidden file, print name (i.e. table name) */
            if (directoryEntry->d_name[0] != '.') {
                printf("%s\n", directoryEntry->d_name);
            }
        }
        closedir(directory);
    }
}