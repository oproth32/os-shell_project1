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
    jobs_init(jobs);   /* make sure table is empty */

    while (1) {
        prompt();
        char *input = get_input();
        if (!input) { putchar('\n'); break; }

        jobs_check_finished(jobs, next_job_id);

        tokenlist *tokens = get_tokens(input);
        if (!tokens || tokens->size == 0) {
            if (tokens) free_tokens(tokens);
            free(input);
            continue;
        }

        /* Part 9: add command to history */
        add_history(input);

        /* Part 9: run built-in command if it matches */
        if (run_internal(tokens, jobs, next_job_id)) {
            free_tokens(tokens);
            free(input);
            continue;
        }

        int run_in_background = 0;
        if (tokens->size > 0 && strcmp(tokens->items[tokens->size - 1], "&") == 0) {
            free(tokens->items[tokens->size - 1]);
            tokens->items[tokens->size - 1] = NULL;
            tokens->size--;
            run_in_background = 1;
            if (tokens->size == 0) { free_tokens(tokens); free(input); continue; }
        }

        int pipelinecheck = 0;
        for (int i = 0; i < (int)tokens->size; ++i) {
            if (tokens->items[i] && strcmp(tokens->items[i], "|") == 0) { pipelinecheck = 1; break; }
        }

        if (!pipelinecheck) {
            if (run_in_background) 
			{
                char display[1024];
                snprintf(display, sizeof(display), "%s", input);
                trim_trailing_amp(display);

                pid_t pid = -1;
                if (pathSearch(tokens, 1, &pid) == 0 && pid > 0) 
				{
                    jobs_add(pid, display, jobs, &next_job_id);
                } else {
                    fprintf(stderr, "failed to start in background\n");
                }
            } else 
			{
                if(pathSearch(tokens, /*fg_bg=*/0, /*out_pid=*/NULL) < 0)
					fprintf(stderr, "command failed\n");
            }
            free_tokens(tokens);
            free(input);
            continue;
        }

        /* ---- PIPELINE HANDLING ---- */
        tokenlist **pipelines = split_by_pipe(tokens);
        if (!pipelines) {
            // syntax error fallback
            pathSearch(tokens, /*fg_bg=*/0, /*out_pid=*/NULL);
            free_tokens(tokens);
            free(input);
            continue;
        }

        if (run_in_background) {
            char display[1024];
            snprintf(display, sizeof(display), "%s", input);
            trim_trailing_amp(display);

            pid_t last_pid = exec_pipeline_bg(pipelines);  // last stage pid
            if (last_pid > 0) {
                jobs_add(last_pid, display, jobs, &next_job_id);
            } else {
                fprintf(stderr, "failed to start in background\n");
            }
        } else {
            if (exec_pipeline(pipelines) != 0) {
                fprintf(stderr, "pipeline failed\n");
            }
        }

        free_split_tokenlists(pipelines);
        free_tokens(tokens);
        free(input);
    }
    return 0;
}
