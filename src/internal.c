#define _POSIX_C_SOURCE 200809L
#include "internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_HISTORY 100
static char *history[MAX_HISTORY];
static int history_size = 0;

void add_history(const char *line) {
    if (!line || !*line) return;
    if (history_size == MAX_HISTORY) {
        free(history[0]);
        memmove(&history[0], &history[1], sizeof(char*) * (MAX_HISTORY - 1));
        history_size--;
    }
    history[history_size++] = strdup(line);
}

void print_history(void) {
    if (history_size == 0) {
        printf("No commands in history.\n");
        return;
    }

    int start = (history_size > 3) ? history_size - 3 : 0;
    for (int i = start; i < history_size; i++) {
        printf("%d %s\n", i + 1, history[i]);
    }
}

int run_internal(tokenlist *tokens, Job *jobs, int next_job_id) {
    if (!tokens || tokens->size == 0) return 0;
    char *cmd = tokens->items[0];

    if (strcmp(cmd, "cd") == 0) {
        if (tokens->size > 2) {
            fprintf(stderr, "cd: too many arguments\n");
            return 1;
        }
        const char *dir = (tokens->size == 2) ? tokens->items[1] : getenv("HOME");
        if (!dir) dir = "/";
        if (chdir(dir) != 0) {
            perror("cd");
        } else {
            char buf[1024];
            if (getcwd(buf, sizeof(buf))) {
                setenv("PWD", buf, 1);
            }
        }
        return 1;
    }

    if (strcmp(cmd, "exit") == 0) {
        print_history();
        int code = 0;
        if (tokens->size > 1) code = atoi(tokens->items[1]);
        exit(code);
    }

    if (strcmp(cmd, "history") == 0) {
        print_history();
        return 1;
    }

    if (strcmp(cmd, "jobs") == 0) {
        int found = 0;
        for (int i = 0; i < MAX_JOBS; i++) {
            if (jobs[i].used) {
                printf("[%d] %d %s\n", jobs[i].id, jobs[i].pid, jobs[i].cmdline);
                found = 1;
            }
        }
        if (!found) {
            printf("No active background processes.\n");
        }
        return 1;
    }

    return 0;
}