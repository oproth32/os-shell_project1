#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "pathSearch.h"


void pathSearch(char *command)
{
    const char *pathEnv = getenv("PATH");
    char *pathBuffer = strdup(pathEnv); // Duplicate PATH to avoid modifying the original
    // Implementation of path search
    printf("Searching for command: %s\n", command);
    printf("Searching through path: %s\n", pathEnv);

    char **paths = getEachPath(pathBuffer);
    int found = 0; // Flag to indicate if command is found
    for (int i = 0; paths[i] != NULL; i++)
    {
        // get the length of each path at runtime
        size_t length = strlen(paths[i]) + strlen(command) + 2;
        char *fullpath = malloc(length); // allocate memory for the full path
        // copy the command and path into fullpath
        strcpy(fullpath, paths[i]);
        strcpy(fullpath + strlen(paths[i]), "/");
        // allocate one byte for the null terminator
        strcpy(fullpath + strlen(paths[i]) + 1, command);
        printf("Checking path: %s\n", fullpath);
        if (access(fullpath, X_OK) == 0) {
            printf("Command found at: %s\n", fullpath);
            found = 1;
            break;
            //  TODO: execv(fullpath); // Execute the command if found
        }
        free(fullpath); // Free the allocated memory
    }
    if (!found) {
        printf("command not found: %s\n", command);
    }
}

char** getEachPath(char *path)
{
    // Implementation of getting each path
    char *token = strtok(path, ":");
    char **paths = NULL;
    int count = 0;
    while (token != NULL)
    {
        // get each path segment
        printf("Path segment: %s\n", token);
        // dynamically allocate memory for each path segment including null terminator
        paths = realloc(paths, sizeof(char *) * (count + 1));
        paths[count] = strdup(token);
        count++;
        token = strtok(NULL, ":");
    }
    // Null terminate the array of paths
    paths = realloc(paths, sizeof(char *) * (count + 1));
    paths[count] = NULL;
    return paths;
}