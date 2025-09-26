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

int main(void) 
{

	static Job jobs[MAX_JOBS];
	static int next_job_id = 1;


	while(1) 
	{
    	prompt();
    	char *input = get_input();
    	if (!input) 
		{ 
			putchar('\n'); 
			break; 
		}

    	jobs_check_finished(jobs, next_job_id);

    	tokenlist *tokens = get_tokens(input);
    	if (!tokens || tokens->size == 0) 
		{
    		free_tokens(tokens);
        	free(input);
        	continue;
    	}

    	/* Background flag handling (unchanged) */
    	int run_in_background = 0;
    	if (tokens->size > 0 && strcmp(tokens->items[tokens->size - 1], "&") == 0) 
		{
        	free(tokens->items[tokens->size - 1]);
        	tokens->items[tokens->size - 1] = NULL;
        	tokens->size--;
        	run_in_background = 1;
        	if (tokens->size == 0) 
			{ 
				free_tokens(tokens); 
				free(input); 
				continue; 
			}
    	}

		/* ---- BUILTIN DISPATCH (BEFORE PIPES) ---- 
		if (is_builtin(tokens)) {
			if (run_in_background) {
				fprintf(stderr, "%s: cannot run built-in in background\n", tokens->items[0]);
				free_tokens(tokens);
				free(input);
				continue;
			}
			// Execute builtin
			if (run_builtin(tokens, jobs, &next_job_id, &hist) == 0) {
				// record in history as a "valid command"
				history_add(&hist, input);
			}
			free_tokens(tokens);
			free(input);
			continue;
		}
		*/

		int pipelinecheck = 0;
		for (int i = 0; i < (int)tokens->size; ++i) 
		{
			if (tokens->items[i] && strcmp(tokens->items[i], "|" ) == 0) 
			{
				pipelinecheck = 1;
				break;
			}

		}

		
		if (!pipelinecheck) 
		{
			pathSearch(tokens);
			free_tokens(tokens);
			free(input);
			continue;
		}
		/* ---- PIPELINE HANDLING ---- */
		tokenlist **pipelines = split_by_pipe(tokens);
		if (run_in_background) 
		{
			char display[1024];
			snprintf(display, sizeof(display), "%s", input);
			trim_trailing_amp(display);

			pid_t last_pid = exec_pipeline_bg(pipelines);
			if (last_pid > 0) 
			{
				jobs_add(last_pid, display, jobs, &next_job_id); // NOTE: &next_job_id
				//history_add(&hist, display);                      // record
			} else 
			{
				fprintf(stderr, "failed to start in background\n");
			}
		} else 
		{
			if (exec_pipeline(pipelines) != 0) 
			{
				fprintf(stderr, "pipeline failed\n");
			} else 
			{
				//history_add(&hist, input); // record successful run
			}
		}

		free_split_tokenlists(pipelines);
		free_tokens(tokens);
		free(input);
	
	}
	return 0;
}