#include "redirection.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include <sys/wait.h>
#include <unistd.h>     /* fork, dup2, execv, access */
#include <sys/types.h>
#include <fcntl.h>      
#include <sys/stat.h>   /* fstat, fchmod, S_ISREG */
#include <errno.h>

/* ---------- parsing ---------- */

static int is_redir_op(const char *s) {
    return s && (strcmp(s, "<") == 0 || strcmp(s, ">") == 0);
}

/* Build argv by copying pointers to existing token strings (no strdup).
   Caller must free the argv pointer array (NOT the strings) later. */
static char **alloc_argv(size_t cap) {
    char **argv = (char **)calloc(cap, sizeof(char *));
    return argv;
}

int parse_redirection_from_tokens(const tokenlist *tokens, CmdParts *parts) {
    parts->in_path = NULL;
    parts->out_path = NULL;
    parts->argv = NULL;
    parts->argc = 0;

    if (!tokens || tokens->size == 0) return 0;

    /* Worst case: every token is part of argv (+ NULL) */
    parts->argv = alloc_argv((size_t)tokens->size + 1);
    if (!parts->argv) return 0;

    size_t out_i = 0;
    for (int i = 0; i < tokens->size; ++i) {
        char *t = tokens->items[i];

        if (strcmp(t, "<") == 0 || strcmp(t, ">") == 0) {
            int is_in = (t[0] == '<');

            /* Need a filename after the operator */
            if (i + 1 >= tokens->size || is_redir_op(tokens->items[i + 1])) {
                fprintf(stderr, "syntax error: %s expects a filename\n", t);
                free(parts->argv);
                parts->argv = NULL;
                return 0;
            }
            char *fname = tokens->items[++i]; /* consume filename */
            if (is_in)  
                parts->in_path = fname;
            else        
                parts->out_path = fname;
            continue;
        }

        /* Normal argv token */
        parts->argv[out_i++] = t;
    }

    parts->argc = out_i;
    parts->argv[out_i] = NULL;

    if (parts->argc == 0) {
        free(parts->argv);
        parts->argv = NULL;
        return 0;
    }
    return 1;
}

/* ---------- redirection helpers (child side) ---------- */

static int setup_input_redir(const char *in_path) {
    int fd = open(in_path, O_RDONLY);
    if (fd < 0) { perror(in_path); return -1; }
    struct stat st;
    if (fstat(fd, &st) == -1 || !S_ISREG(st.st_mode)) {
        fprintf(stderr, "%s: not a regular file\n", in_path);
        close(fd);
        errno = EINVAL;
        return -1;
    }
    if (dup2(fd, STDIN_FILENO) == -1) { 
        perror("dup2 stdin"); 
        close(fd); 
        return -1; 
    }
    close(fd);
    return 0;
}

static int setup_output_redir(const char *out_path) {
    int fd = open(out_path, O_WRONLY | O_CREAT | O_TRUNC, 0600); /* -rw------- */
    if (fd < 0) { 
        perror(out_path);
         return -1; 
    }
    
    if (dup2(fd, STDOUT_FILENO) == -1) {
        perror("dup2 stdout");
        close(fd);
        return -1;
    }
    close(fd);
    return 0;
}

/* ---------- fork/exec wrapper ---------- */
void exec_external_with_redir(const char *path_to_exec, CmdParts *parts) {
    if (!path_to_exec || !parts || !parts->argv || !parts->argv[0]) {
        if (parts && parts->argv) 
            free(parts->argv);
        return;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        free(parts->argv);
        return;
    }
    if (pid == 0) {
        /* CHILD: set up redirections first */
        if (parts->in_path  && setup_input_redir(parts->in_path) == -1) 
            _exit(1);
        if (parts->out_path && setup_output_redir(parts->out_path) == -1) 
            _exit(1);

        execv(path_to_exec, parts->argv);
        perror(parts->argv[0]);
        _exit(127);
    } else {
        /* PARENT: wait (background support comes later) */
        int status;
        (void)waitpid(pid, &status, 0);
        free(parts->argv); /* free only the argv pointer array we allocated */
    }
}

// Spawn with redirection but DO NOT wait; returns child's pid (or -1 on error).
pid_t spawn_external_with_redir(const char *path_to_exec, CmdParts *parts) {
    if (!path_to_exec || !parts || !parts->argv || !parts->argv[0]) {
        if (parts && parts->argv) free(parts->argv);
        return -1;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        free(parts->argv);
        return -1;
    }
    if (pid == 0) {
        /* CHILD: set up redirections first */
        if (parts->in_path  && setup_input_redir(parts->in_path)  == -1) _exit(1);
        if (parts->out_path && setup_output_redir(parts->out_path) == -1) _exit(1);

        execv(path_to_exec, parts->argv);
        perror(parts->argv[0]);
        _exit(127);
    }

    /* PARENT: do NOT wait here. Caller is responsible for freeing parts->argv. */
    return pid;
}
