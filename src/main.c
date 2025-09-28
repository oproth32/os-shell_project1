#include "lexer.h"
#include "prompt.h"
#include "environmentVariables.h"
#include "pathSearch.h"
#include "pipeline.h"
#include "background.h"
#include "redirection.h"
#include "internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
    static Job jobs[MAX_JOBS];
    static int next_job_id = 1;
    jobs_init(jobs);   /* ensure empty table */

    for (;;) {
        /* Reap any finished background children first so '+ done ...' appears before prompt */
        jobs_check_finished(jobs, next_job_id);

        /* Prompt & read */
        prompt();
        char *input = get_input();
        if (!input) { putchar('\n'); break; }

        /* Tokenize */
        tokenlist *tokens = get_tokens(input);
        if (!tokens || tokens->size == 0) {
            if (tokens) free_tokens(tokens);
            free(input);
            continue;
        }

        /* ---- BUILT-INS (Part 9): run in parent, no & or pipes ----
           NOTE: run_internal should reject pipelines/& for built-ins and
           print an error itself when applicable. It should also handle:
             - exit: wait for bg jobs + print last 3 valid commands
             - cd:   as spec
             - jobs: list active bg jobs
           It should return 1 if it handled the command (whether success or error),
           0 if not a builtin. */
        if (run_internal(tokens, jobs, &next_job_id)) {
            /* If you want built-ins to count toward history, add here: */
            add_history(input);
            free_tokens(tokens);
            free(input);
            continue;
        }

        /* Background flag at end */
        int run_in_background = 0;
        if (tokens->size > 0 && strcmp(tokens->items[tokens->size - 1], "&") == 0) {
            free(tokens->items[tokens->size - 1]);
            tokens->items[tokens->size - 1] = NULL;
            tokens->size--;
            run_in_background = 1;
            if (tokens->size == 0) { free_tokens(tokens); free(input); continue; }
        }

        /* Does the line contain any '|'? */
        int has_pipe = 0;
        for (int i = 0; i < (int)tokens->size; ++i) {
            if (tokens->items[i] && strcmp(tokens->items[i], "|") == 0) { has_pipe = 1; break; }
        }

        if (!has_pipe) {
            /* ---------- Single command ---------- */
            if (run_in_background) {
                char display[1024];
                snprintf(display, sizeof(display), "%s", input);
                trim_trailing_amp(display);

                pid_t pid = -1;
                if (pathSearch(tokens, /*bg*/1, &pid) == 0 && pid > 0) {
                    jobs_add(pid, display, jobs, &next_job_id);
                    add_history(display);    /* record valid command line (cleaned) */
                } else {
                    fprintf(stderr, "failed to start in background\n");
                }
            } else {
                (void)pathSearch(tokens, /*fg*/0, /*out_pid*/NULL);
                /* pathSearch() prints its own errors; still record for history as 'valid' */
                add_history(input);
            }
            free_tokens(tokens);
            free(input);
            continue;
        }

        /* ---------- Pipeline path ---------- */
        tokenlist **segments = split_by_pipe(tokens);
        if (!segments) {
            /* split_by_pipe already printed a syntax error */
            free_tokens(tokens);
            free(input);
            continue;
        }

        if (run_in_background) {
            char display[1024];
            snprintf(display, sizeof(display), "%s", input);
            trim_trailing_amp(display);

            pid_t last_pid = exec_pipeline_bg(segments);  /* last stage pid */
            if (last_pid > 0) {
                jobs_add(last_pid, display, jobs, &next_job_id);
                add_history(display);
            } else {
                fprintf(stderr, "failed to start in background\n");
            }
        } else {
            (void)exec_pipeline(segments);  /* prints its own errors */
            add_history(input);
        }

        free_split_tokenlists(segments);
        free_tokens(tokens);
        free(input);
    }
    return 0;
}
