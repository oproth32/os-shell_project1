#include "pipeline.h"
#include "redirection.h"
#include "pathSearch.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>      // fork, pipe, dup2, close, execv
#include <sys/wait.h>    // waitpid
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif


void free_split_tokenlists(tokenlist **lists) 
{
    if (!lists) 
        return;
    for (int i = 0; lists[i]; ++i) 
    {
        if (lists[i]->items) free(lists[i]->items);
        free(lists[i]);
    }
    free(lists);
}

/* Split by '|' with basic syntax checks. Borrow token pointers; allocate only the
   (resizable) items arrays and the tokenlist structs for each segment. */
tokenlist **split_by_pipe(const tokenlist *tokens) 
{
    if (!tokens || tokens->size == 0) 
        return NULL;

    int max_cmds = tokens->size / 2 + 2;        // worst-case + NULL
    tokenlist **result = calloc((size_t)max_cmds, sizeof(*result));
    if (!result) 
        return NULL;

    result[0] = calloc(1, sizeof(*result[0]));
    if (!result[0]) 
    { 
        free(result); 
        return NULL; 
    }

    int seg = 0;

    for (int i = 0; i < (int)tokens->size; ++i) 
    {
        char *tok = tokens->items[i];

        if (strcmp(tok, "|") == 0) 
        {
            // reject empty segment (leading, consecutive)
            if (!result[seg] || result[seg]->size == 0) 
            {
                fprintf(stderr, "syntax error: empty command near '|'\n");
                free_split_tokenlists(result);
                return NULL;
            }
            // start next segment
            seg++;
            if (seg >= max_cmds - 1) 
            {
                fprintf(stderr, "too many pipeline segments\n");
                free_split_tokenlists(result);
                return NULL;
            }
            result[seg] = calloc(1, sizeof(*result[seg]));
            if (!result[seg]) 
            { 
                free_split_tokenlists(result); 
                return NULL; 
            }
            continue;
        }

        tokenlist *cur = result[seg];
        char **new_items = realloc(cur->items, sizeof(char*) * (cur->size + 1));
        if (!new_items) 
        { 
            free_split_tokenlists(result); 
            return NULL; 
        }
        cur->items = new_items;
        cur->items[cur->size++] = tok;          // borrow pointer
    }

    // reject trailing pipe (last segment empty)
    if (result[seg]->size == 0) 
    {
        fprintf(stderr, "syntax error: trailing '|'\n");
        free_split_tokenlists(result);
        return NULL;
    }

    result[seg + 1] = NULL;
    return result;
}

/* --------------------------------------------------------------------------------
   Concurrent pipeline implementation
   - Parse argv per stage (no string duplication; free argv arrays in parent)
   - Resolve exec path per stage
   - Create N-1 pipes
   - Fork N children; in each child, dup2() appropriate pipe ends; execv()
   - Parent closes all pipe FDs. Foreground waits; background returns last PID.
   -------------------------------------------------------------------------------- */



/* Free argv arrays for all prepared stages (parent side). */
static void free_stage_argvs(Stage *stages, int n) 
{
    if (!stages) 
        return;
    for (int i = 0; i < n; ++i) 
    {
        if (stages[i].parts.argv) 
        {
            free(stages[i].parts.argv);
            stages[i].parts.argv = NULL;
        }
    }
}

/* Prepare all stages: parse redirection -> argv, and resolve exec path.
   Returns number of stages on success; -1 on failure (with errors printed). */
static int prepare_stages(tokenlist **segments, Stage **out_stages) 
{
    if (!segments || !segments[0] || !out_stages) 
        return -1;

    // Count stages
    int n = 0;
    while (segments[n]) 
        n++;
    if (n <= 0) 
        return -1;

    Stage *stages = calloc((size_t)n, sizeof(Stage));
    if (!stages) 
    { 
        perror("calloc"); 
        return -1;
    }

    // Parse and resolve each stage
    for (int i = 0; i < n; ++i) {
        // 1) Parse redirection into argv/in/out
        if (!parse_redirection_from_tokens(segments[i], &stages[i].parts)) 
        {
            // parse already printed syntax error
            free_stage_argvs(stages, i);
            free(stages);
            return -1;
        }

        // Optional safeguard: disallow mixing pipes with redirection except:
        // allow '<' ONLY on the first stage, and '>' ONLY on the last stage.
        // Your spec says piping and redirection won't appear together, so you
        // can comment this out if you prefer.
        if ((stages[i].parts.in_path  && i != 0) ||
            (stages[i].parts.out_path && segments[i+1] != NULL))
        {
            fprintf(stderr, "error: redirection only allowed on the ends of a pipeline\n");
            free_stage_argvs(stages, n);
            free(stages);
            return -1;
        }

        // 2) Resolve executable path for argv[0]
        if (!stages[i].parts.argv || !stages[i].parts.argv[0]) 
        {
            fprintf(stderr, "syntax error: empty command in pipeline\n");
            free_stage_argvs(stages, n);
            free(stages);
            return -1;
        }
        if (!resolve_command(stages[i].parts.argv[0],
            stages[i].exec_path, sizeof(stages[i].exec_path))) 
        {
            fprintf(stderr, "%s: command not found\n", stages[i].parts.argv[0]);
            free_stage_argvs(stages, n);
            free(stages);
            return -1;
        }
    }

    *out_stages = stages;
    return n;
}

/* Close all pipe FDs helper. */
static void close_all_pipes(int *pfds, int n_pipes) 
{
    if (!pfds) 
        return;
    for (int i = 0; i < n_pipes; ++i) 
    {
        if (pfds[2*i] >= 0) 
            close(pfds[2*i]);     // read end
        if (pfds[2*i+1] >= 0) 
            close(pfds[2*i+1]);   // write end
    }
}

/* In child: apply allowed redirection (only if at ends), then exec. */
static void apply_end_redirections_and_exec(const Stage *stages, int idx, int n) 
{
    // If first stage has "<", apply it (stdin)
    if (idx == 0 && stages[idx].parts.in_path) 
    {
        int fd = open(stages[idx].parts.in_path, O_RDONLY);
        if (fd < 0) 
        { 
            perror(stages[idx].parts.in_path);
            _exit(1);
        }
        struct stat st;
        if (fstat(fd, &st) == -1 || !S_ISREG(st.st_mode)) 
        {
            fprintf(stderr, "%s: not a regular file\n", stages[idx].parts.in_path);
            close(fd); _exit(1);
        }
        if (dup2(fd, STDIN_FILENO) == -1) 
        { 
            perror("dup2 stdin"); 
            close(fd); 
            _exit(1); 
        }
        close(fd);
    }
    // If last stage has ">", apply it (stdout)
    if (idx == n-1 && stages[idx].parts.out_path) 
    {
        int fd = open(stages[idx].parts.out_path, O_WRONLY|O_CREAT|O_TRUNC, 0600);
        if (fd < 0) 
        { 
            perror(stages[idx].parts.out_path); 
            _exit(1); 
        }
        if (dup2(fd, STDOUT_FILENO) == -1) 
        { 
            perror("dup2 stdout"); 
            close(fd); 
            _exit(1); 
        }
        close(fd);
    }

    execv(stages[idx].exec_path, stages[idx].parts.argv);
    perror(stages[idx].parts.argv[0]);
    _exit(127);
}

/* Core launcher shared by foreground/background entry points.
   If background == 0: wait for all children, return 0 on success, -1 on fail.
   If background != 0: do NOT wait; return last stage pid (or -1 on fail). */
static long launch_pipeline(tokenlist **segments, int background) 
{
    Stage *stages = NULL;
    int n = prepare_stages(segments, &stages);
    if (n < 0) return -1;

    // Single stage: no pipes needed.
    if (n == 1)
    {
        pid_t pid = fork();
        if (pid < 0) 
        {
            perror("fork");
            free_stage_argvs(stages, n);
            free(stages);
            return -1;
        }
        if (pid == 0) 
        {
            // child: just apply end redirs & exec
            apply_end_redirections_and_exec(stages, 0, 1);
        }
        // parent
        long ret;
        if (!background) 
        {
            int st;
            if (waitpid(pid, &st, 0) < 0) 
                perror("waitpid");
            ret = 0;  // foreground success
        } else 
        {
            ret = (long)pid;  // background: return child's pid
        }
        free_stage_argvs(stages, n);
        free(stages);
        return ret;
    }

    // Multi-stage: create N-1 pipes
    int n_pipes = n - 1;
    int *pfds = calloc((size_t)(2*n_pipes), sizeof(int));
    if (!pfds) 
    {
        perror("calloc");
        free_stage_argvs(stages, n);
        free(stages);
        return -1;
    }

    for (int i = 0; i < n_pipes; ++i) 
    {
        if (pipe(&pfds[2*i]) < 0) 
        {
            perror("pipe");
            // close any previously opened
            for (int k = 0; k < i; ++k) 
            {
                close(pfds[2*k]); close(pfds[2*k+1]);
            }
            free(pfds);
            free_stage_argvs(stages, n);
            free(stages);
            return -1;
        }
    }

    // Fork N children
    for (int i = 0; i < n; ++i) 
    {
        pid_t pid = fork();
        if (pid < 0) 
        {
            perror("fork");
            // Parent cleanup: close all pipes, kill any children if you want (optional),
            // and wait for those already started to avoid zombies.
            close_all_pipes(pfds, n_pipes);
            for (int j = 0; j < i; ++j) (void)waitpid(stages[j].pid, NULL, 0);
            free(pfds);
            free_stage_argvs(stages, n);
            free(stages);
            return -1;
        }

        if (pid == 0) 
        {
            // CHILD i: wire pipe ends
            // If not first: stdin <- read end of previous pipe
            if (i > 0) 
            {
                if (dup2(pfds[2*(i-1)], STDIN_FILENO) == -1) 
                { 
                    perror("dup2 stdin"); 
                    _exit(1); 
                }
            }
            // If not last: stdout -> write end of next pipe
            if (i < n-1) 
            {
                if (dup2(pfds[2*i + 1], STDOUT_FILENO) == -1) 
                { 
                    perror("dup2 stdout"); 
                    _exit(1); 
                }
            }
            // Close ALL pipe fds in child (we've duplicated what we need)
            for (int k = 0; k < n_pipes; ++k) 
            {
                close(pfds[2*k]);
                close(pfds[2*k + 1]);
            }

            // Apply optional end redirections (< only on first, > only on last)
            apply_end_redirections_and_exec(stages, i, n);
        }

        // PARENT: record pid and continue
        stages[i].pid = pid;
    }

    // Parent: close all pipe FDs so children can see EOF
    close_all_pipes(pfds, n_pipes);
    free(pfds);

    long ret;
    if (!background) 
    {
        // Foreground: wait for all children
        for (int i = 0; i < n; ++i) 
        {
            int st;
            if (waitpid(stages[i].pid, &st, 0) < 0) 
                perror("waitpid");
        }
        ret = 0;
    } else 
    {
        // Background: return the LAST stage's PID 
        ret = (long)stages[n-1].pid;
    }

    free_stage_argvs(stages, n);
    free(stages);
    return ret;
}

/* Public entry points */

int exec_pipeline(tokenlist **segments) 
{
    long rc = launch_pipeline(segments, /*background=*/0);
    return (rc < 0) ? -1 : 0;
}

pid_t exec_pipeline_bg(tokenlist **segments) 
{
    long pid = launch_pipeline(segments, /*background=*/1);
    return (pid < 0) ? -1 : (pid_t)pid;
}
