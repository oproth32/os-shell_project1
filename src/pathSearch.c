#include "pathSearch.h"
#include "redirection.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include <unistd.h> // access 
#include <sys/stat.h> // stat, S_ISREG 

// -------- helpers to resolve executable path -------- 
#define PATH_MAX 4096

static int has_slash(const char *s) {
    return s && strchr(s, '/') != NULL;
}

// Resolve an executable's full path.
// Returns 1 and writes into out_path on success, 0 on failure. 
int resolve_command(const char *cmd, char *out_path, size_t out_sz) {
    if (!cmd || !*cmd || !out_path || out_sz == 0)
        return 0;

    // If cmd contains '/', treat it as a direct path (like shells do). 
    if (strchr(cmd, '/')) {
        if (access(cmd, X_OK) == 0) {
            struct stat st;
            if (stat(cmd, &st) == 0 && S_ISREG(st.st_mode)) {
                if (snprintf(out_path, out_sz, "%s", cmd) < (int)out_sz)
                    return 1;
            }
        }
        return 0;
    }

    // Search PATH. 
    const char *path = getenv("PATH");
    if (!path || !*path)
        path = "/bin:/usr/bin";

    const char *p = path;
    char candidate[4096];

    while (*p) {
        // Find next ':' or end. 
        const char *colon = strchr(p, ':');
        size_t dir_len = colon ? (size_t)(colon - p) : strlen(p);

        if (dir_len == 0) {
            // Empty entry => current directory "." 
            if (snprintf(candidate, sizeof(candidate), "./%s", cmd) >= 0 &&
                access(candidate, X_OK) == 0) {
                struct stat st;
                if (stat(candidate, &st) == 0 && S_ISREG(st.st_mode)) {
                    if (snprintf(out_path, out_sz, "%s", candidate) < (int)out_sz)
                        return 1;
                }
            }
        } else {
            // Build "<dir>/<cmd>" safely. 
            if (dir_len >= sizeof(candidate))
                dir_len = sizeof(candidate) - 1;
            if (snprintf(candidate, sizeof(candidate), "%.*s/%s", (int)dir_len, p, cmd) >= 0 &&
                access(candidate, X_OK) == 0) {
                struct stat st;
                if (stat(candidate, &st) == 0 && S_ISREG(st.st_mode)) {
                    if (snprintf(out_path, out_sz, "%s", candidate) < (int)out_sz)
                        return 1;
                }
            }
        }

        if (!colon)
            break;
        p = colon + 1;
    }

    return 0;
}

// -------- main entry -------- 

void pathSearch(tokenlist *tokens) {
    if (!tokens || tokens->size == 0) return;

    // 1) Parse out < and >, build argv[] 
    CmdParts parts;
    if (!parse_redirection_from_tokens(tokens, &parts)) {
        return; // syntax error already printed 
    }

    const char *command = parts.argv[0];

    // 2) If command contains '/', treat as direct path (no PATH search) 
    if (has_slash(command)) {
        if (access(command, X_OK) == 0) {
            exec_external_with_redir(command, &parts);
        } else {
            fprintf(stderr, "%s: command not found or not executable\n", command);
            free(parts.argv);
        }
        return;
    }

    // 3) PATH search 
    char resolved[PATH_MAX];
    if (resolve_command(command, resolved, sizeof(resolved))) {
        exec_external_with_redir(resolved, &parts);
    } else {
        fprintf(stderr, "%s: command not found\n", command);
        free(parts.argv);
    }
}
