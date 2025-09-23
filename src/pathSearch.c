#include "pathSearch.h"
#include "redirection.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include <unistd.h>     /* access */
#include <sys/stat.h>   /* stat, S_ISREG */

/* -------- helpers to resolve executable path -------- */
#define PATH_MAX 4096

static int has_slash(const char *s) {
    return s && strchr(s, '/') != NULL;
}

/* Search $PATH for cmd; write full path to resolved on success, return 1/0. */
static int resolve_via_path(const char *cmd, char resolved[PATH_MAX]) {
    const char *path = getenv("PATH");
    if (!cmd || !*cmd) return 0;
    if (!path) path = "";

    size_t len = strlen(path);
    char *copy = malloc(len+1);
    if (copy == NULL)
	perror("malloc failed");

    strcpy(copy, path);
    if (!copy) return 0;

    int ok = 0;
    for (char *seg = strtok(copy, ":"); seg; seg = strtok(NULL, ":")) {
        const char *dir = (*seg == '\0') ? "." : seg; /* empty segment == current dir */
        if (snprintf(resolved, PATH_MAX, "%s/%s", dir, cmd) >= (int)PATH_MAX) {
            continue; /* path too long, skip */
        }
        if (access(resolved, X_OK) == 0) {
            struct stat st;
            if (stat(resolved, &st) == 0 && S_ISREG(st.st_mode)) {
                ok = 1;
                break;
            }
        }
    }
    free(copy);
    return ok;
}

/* -------- main entry -------- */

void pathSearch(tokenlist *tokens) {
    if (!tokens || tokens->size == 0) return;

    /* 1) Parse out < and >, build argv[] */
    CmdParts parts;
    if (!parse_redirection_from_tokens(tokens, &parts)) {
        return; /* syntax error already printed */
    }

    const char *command = parts.argv[0];

    /* 2) If command contains '/', treat as direct path (no PATH search) */
    if (has_slash(command)) {
        if (access(command, X_OK) == 0) {
            exec_external_with_redir(command, &parts);
        } else {
            fprintf(stderr, "%s: command not found or not executable\n", command);
            free(parts.argv);
        }
        return;
    }

    /* 3) PATH search */
    char resolved[PATH_MAX];
    if (resolve_via_path(command, resolved)) {
        exec_external_with_redir(resolved, &parts);
    } else {
        fprintf(stderr, "%s: command not found\n", command);
        free(parts.argv);
    }
}
