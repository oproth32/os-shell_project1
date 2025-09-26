#include "prompt.h"
#include <stdlib.h>
#include <stdio.h>

// Print the shell prompt in the format user@machine:currentDirectory>
void prompt() {
    char *username = getenv("USER");
    char* machine = getenv("HOSTNAME") ? getenv("HOSTNAME") : getenv("MACHINE");
    char* currentDirectory = getenv("PWD");
    if (currentDirectory == NULL)
        currentDirectory = "unknown";
    if (machine == NULL)
        machine = "unknown";
    if (username == NULL)
        username = "unknown";
    printf("%s@%s:%s> ", username, machine, currentDirectory);
}