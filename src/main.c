#include "lexer.h"
#include "prompt.h"
#include "environmentVariables.h"
#include "pathSearch.h"
#include "pipeline.h"
#include "background.h"
#include "redirection.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main()
{
	while (1) {
		// print the username, machine name and current directory like in a normal shell
		// e.g. user@machine:/current/directory
		prompt();

		/* input contains the whole command
		 * tokens contains substrings from input split by spaces
		 */

		char *input = get_input();
		// printf("whole input: %s\n", input);

		// check how many background jobs have finished
		// static array of jobs
		static Job jobs[MAX_JOBS];
		static int next_job_id = 1;
		jobs_check_finished(jobs, next_job_id);

		tokenlist *tokens = get_tokens(input);
		int run_in_background = 0;

		// checking for the & command to run in background
		// the '&' can only be at the end of the command
		if (tokens->size > 0 && strcmp(tokens->items[tokens->size - 1], "&") == 0)
		{
			if (tokens->size == 0) {  // lone '&'
				free_tokens(tokens);
				free(input);
				continue;
			}
			// remove the '&' from the tokens list
			free(tokens->items[tokens->size - 1]);
			tokens->items[tokens->size - 1] = NULL;
			tokens->size--;
			run_in_background = 1;
		}

		// split tokens by pipe character
		tokenlist **pipelines = split_by_pipe(tokens);

		/* this is the output of each token for each pipeline

		for (int i = 0; pipelines[i]; i++) {
			printf("Pipeline %d:\n", i);
			for (int j = 0; j < pipelines[i]->size; j++) {
				printf("token %d: (%s)\n", j, pipelines[i]->items[j]);
			}
		}
		*/

		 // if there are no pipelines, call path search function
		 // this also handles syntax errors like | cmd or cmd1 || cmd2
		if (!pipelines) {
			// split_by_pipe already printed a syntax error if any
			pathSearch(tokens); // call path search function
			free(input);
			free_tokens(tokens);
			continue;
		}

		// execute the pipeline
		 // if run_in_background is true, run in background
		 // otherwise, run in foreground
		 // remember to add the job to the jobs list if running in background
		 // and print the job id and pid of the last command in the pipeline

		 // if there is only one command in the pipeline, just call path search function
		 // with that command's tokens
		if (run_in_background) {
			char display[1024];
            snprintf(display, sizeof(display), "%s", input);
            trim_trailing_amp(display);

            pid_t pid = exec_pipeline_filebacked_bg(pipelines);
            if (pid > 0) {
                jobs_add(pid, display, jobs, next_job_id);
            } 
			else {
                fprintf(stderr, "failed to start in background\n");
            }
        } 
		else {
            if (exec_pipeline_filebacked(pipelines) != 0) {
                fprintf(stderr, "pipeline failed\n");
            }
		}

		// Free the split array (remember: it only owns the items arrays + tokenlist structs)
		free_split_tokenlists(pipelines);

		/*
		for (int i = 0; i < tokens->size; i++) {
			printf("token %d: (%s)\n", i, tokens->items[i]);
		}
		*/

		free(input);
		free_tokens(tokens);
	}

	return 0;
}


