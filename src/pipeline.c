#include "lexer.h"
#include "redirection.h"     // <- use your actual function names here
#include "pathSearch.h"      // your single-command launcher (execs)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>          // fork, exec, dup2
#include <sys/wait.h>        // waitpid
#include <errno.h>
#include <fcntl.h>

/* Free the array returned by split_by_pipe (frees tokenlist structs and their items arrays).
   NOTE: It does NOT free the token strings themselves, since they are borrowed from the original tokens. */
void free_split_tokenlists(tokenlist **lists) {
    if (!lists) 
        return;
    for (int i = 0; lists[i]; ++i) {
        free(lists[i]->items);
        free(lists[i]);
    }
    free(lists);
}

/* Split tokens by '|' into an array of tokenlist* (NULL-terminated).
   Each sub-tokenlist borrows string pointers from the input; it owns only its items array.
   Returns NULL on error (invalid syntax or OOM). */
tokenlist **split_by_pipe(const tokenlist *tokens) {
    if (!tokens || tokens->size == 0) 
        return NULL;

    /* Worst-case number of commands: word | word | word ... => ~ size/2 + 1.
       +1 for safety, +1 for NULL terminator. */
    int max_cmds = tokens->size / 2 + 2;
    tokenlist **result = calloc((size_t)max_cmds, sizeof(*result));
    if (!result) 
        return NULL;

    /* Start first segment */
    result[0] = calloc(1, sizeof(*result[0]));
    if (!result[0]) { 
        free(result); 
        return NULL; 
    }
    int seg = 0; // index of current segment

    for (int i = 0; i < tokens->size; ++i) {
        char *tok = tokens->items[i];

        if (strcmp(tok, "|") == 0) {
            /* Reject consecutive pipes or leading pipe: empty segment is invalid */
            if (!result[seg] || result[seg]->size == 0) {
                fprintf(stderr, "syntax error: empty command near '|'\n");
                free_split_tokenlists(result);
                return NULL;
            }
            /* Start next segment */
            seg++;
            if (seg >= max_cmds - 1) { /* ensure room for NULL terminator */
                fprintf(stderr, "too many pipeline segments\n");
                free_split_tokenlists(result);
                return NULL;
            }
            result[seg] = calloc(1, sizeof(*result[seg]));
            if (!result[seg]) {
                free_split_tokenlists(result);
                return NULL;
            }
            continue;
        }

        /* Append token to current segment */
        tokenlist *cur = result[seg];
        char **new_items = realloc(cur->items, sizeof(char *) * (cur->size + 1));
        if (!new_items) {
            free_split_tokenlists(result);
            return NULL;
        }
        cur->items = new_items;
        cur->items[cur->size] = tok;  /* borrow pointer to original token */
        cur->size += 1;
    }

    /* Reject trailing pipe: last segment must not be empty */
    if (result[seg]->size == 0) {
        fprintf(stderr, "syntax error: trailing '|'\n");
        free_split_tokenlists(result);
        return NULL;
    }

    /* NULL-terminate the array */
    result[seg + 1] = NULL;
    return result;
}

static int mktemp_path(char *buf, size_t n) {
    // Create a unique temp file path; the file will be created/closed by mkstemp
    // and we’ll re-open it later for read/write.
    snprintf(buf, n, "/tmp/ossh_%ld_XXXXXX", (long)getpid());
    int fd = mkstemp(buf);
    if (fd < 0) 
        return -1;
    close(fd);               // we’ll reopen with redirect functions as needed
    return 0;
}
/* ---- path resolver (since we won't use pathSearch here) ---- */
static int resolve_fullpath(const char *cmd, char out_path[], size_t out_sz) {
    if (!cmd || !*cmd) 
        return 0;

    if (strchr(cmd, '/')) {
        if (access(cmd, X_OK) == 0) {
            snprintf(out_path, out_sz, "%s", cmd);
            return 1;
        }
        return 0;
    }

    const char *path = getenv("PATH");
    if (!path) 
        path = "/bin:/usr/bin";
    const char *p = path;
    char buf[4096];

    while (*p) {
        const char *colon = strchr(p, ':');
        size_t len = colon ? (size_t)(colon - p) : strlen(p);

        if (len == 0) { /* empty entry == current dir */
            if (snprintf(buf, sizeof(buf), "./%s", cmd) >= 0 && access(buf, X_OK) == 0) {
                snprintf(out_path, out_sz, "%s", buf);
                return 1;
            }
        } else {
            if (len >= sizeof(buf)) 
                len = sizeof(buf) - 1;
            if (snprintf(buf, sizeof(buf), "%.*s/%s", (int)len, p, cmd) >= 0 && access(buf, X_OK) == 0) {
                snprintf(out_path, out_sz, "%s", buf);
                return 1;
            }
        }
        if (!colon) 
            break;
        p = colon + 1;
    }
    return 0;
}

/* Build CmdParts from a segment and optionally force stdin/stdout via files. */
static int make_parts(const tokenlist *segment, const char *forced_in, const char *forced_out, CmdParts *out) {
    if (!parse_redirection_from_tokens(segment, out)) {
        return 0;
    }

    /* Only disallow when the SAME side is forced by the pipeline. 
    Allow:  - '<' on first stage (forced_out set, forced_in NULL)
            - '>' on last stage  (forced_in set, forced_out NULL)
    Disallow: user '<' when forced_in, or user '>' when forced_out. */
    if ((forced_in && out->in_path) ||   /* two inputs: conflict */
        (forced_out && out->out_path)) {  /* two outputs: conflict */
        fprintf(stderr, "error: cannot redirect %s when pipeline %s already assigned\n",
                (forced_in && out->in_path) ? "stdin" : "stdout",
                (forced_in && out->in_path) ? "input is" : "output is");
        free(out->argv);
        out->argv = NULL;
        return 0;
    }

    if (forced_in)  
        out->in_path = (char *)forced_in;
    if (forced_out) 
        out->out_path = (char *)forced_out;
    return 1;
}

/* Run one stage via exec_external_with_redir (which forks & dup2s in the child). */
static int run_stage(const tokenlist *segment, const char *forced_in, const char *forced_out) {
    CmdParts parts;
    if (!make_parts(segment, forced_in, forced_out, &parts)) 
        return -1;

    char full[4096];
    if (!resolve_fullpath(parts.argv[0], full, sizeof(full))) {
        fprintf(stderr, "%s: command not found\n", parts.argv[0]);
        free(parts.argv);
        return -1;
    }

    exec_external_with_redir(full, &parts);
    /* exec_external_with_redir frees parts.argv in the parent */
    return 0;
}

/* Execute a pipeline using temporary files between stages.
 * segments: NULL-terminated array from split_by_pipe (each tokenlist* is a command)
 * Returns 0 on success, -1 on error.
 */
int exec_pipeline_filebacked(tokenlist **segments) {
    if (!segments || !segments[0]) return -1;

    /* Count stages */
    int n = 0;
    while (segments[n]) 
        n++;

    if (n == 1) {
        /* No chaining; just run it with whatever redirections it contains. */
        return run_stage(segments[0], NULL, NULL);
    }

    // Create one temp file for the whole pipeline
    char **temps = calloc((size_t)(n - 1), sizeof(char *));
    if (!temps) { 
        perror("calloc"); 
        return -1; 
    }

    for (int i = 0; i < n - 1; i++) {
        temps[i] = calloc(1, 64);
        if (!temps[i] || mktemp_path(temps[i], 64) != 0) {
            perror("mkstemp");
            for (int k = 0; k <= i; k++)
            {
                if (temps[k]) {
                    unlink(temps[k]);
                    free(temps[k]);
                }
            }
            free(temps);
            return -1;
        }
    }

    int rc = 0;

    /* First stage: stdout -> temps[0] */
    if (rc == 0) {
        if (run_stage(segments[0], /*in*/NULL, /*out*/temps[0]) != 0)
            rc = -1;
    }

    /* Middles: stdin <- temps[i-1], stdout -> temps[i] */
    for (int i = 1; rc == 0 && i < n - 1; i++) {
        if (run_stage(segments[i], /*in*/temps[i - 1], /*out*/temps[i]) != 0)
            rc = -1;
    }

    /* Last: stdin <- temps[n-2], stdout -> terminal */
    if (rc == 0) {
        if (run_stage(segments[n - 1], /*in*/temps[n - 2], /*out*/NULL) != 0)
            rc = -1;
    }

    /* Cleanup temps */
    for (int i = 0; i < n - 1; i++) {
        if (temps[i]) {
            unlink(temps[i]);
            free(temps[i]);
        }
    } 
    free(temps);

    return rc;
}

static pid_t spawn_stage_bg(const tokenlist *segment, const char *forced_in, const char *forced_out) {
    CmdParts parts;
    if (!make_parts(segment, forced_in, forced_out, &parts)) return -1;
    char full[4096];
    if (!resolve_fullpath(parts.argv[0], full, sizeof(full))) { fprintf(stderr, "%s: command not found\n", parts.argv[0]); free(parts.argv); return -1; }
    pid_t pid = spawn_external_with_redir(full, &parts);
    if (parts.argv) free(parts.argv);
    return pid;
}

pid_t exec_pipeline_filebacked_bg(tokenlist **segments) {
    if (!segments || !segments[0]) 
        return -1;
    int n = 0; 
    while (segments[n]) 
        n++;

    if (n == 1) return spawn_stage_bg(segments[0], NULL, NULL);

    char **temps = calloc((size_t)(n - 1), sizeof(char *));
    if (!temps) { perror("calloc"); return -1; }
    for (int i = 0; i < n - 1; i++) {
        temps[i] = calloc(1, 64);
        if (!temps[i] || mktemp_path(temps[i], 64) != 0) {
            perror("mkstemp");
            for (int k = 0; k <= i; k++) { if (temps[k]) { unlink(temps[k]); free(temps[k]); } }
            free(temps); return -1;
        }
    }

    // Foreground first N-1 stages to produce the final temp input
    if (run_stage(segments[0], NULL, temps[0]) != 0) goto fail;
    for (int i = 1; i < n - 1; i++) {
        if (run_stage(segments[i], temps[i - 1], temps[i]) != 0) goto fail;
        // optional: unlink(temps[i - 1]);
    }

    // Background last stage (stdin <- temps[n-2])
    {
        pid_t pid = spawn_stage_bg(segments[n - 1], temps[n - 2], NULL);
        for (int i = 0; i < n - 1; i++) { if (temps[i]) { unlink(temps[i]); free(temps[i]); } }
        free(temps);
        return pid;
    }

fail:
    for (int i = 0; i < n - 1; i++) { if (temps[i]) { unlink(temps[i]); free(temps[i]); } }
    free(temps);
    return -1;
}
