#include "lexer.h"
#include "prompt.h"
#include "environmentVariables.h"
#include "pathSearch.h"
#include "pipeline.h"
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
		printf("whole input: %s\n", input);

		tokenlist *tokens = get_tokens(input);
		tokenlist **pipelines = split_by_pipe(tokens);

		for (int i = 0; pipelines[i]; i++) {
			printf("Pipeline %d:\n", i);
			for (int j = 0; j < pipelines[i]->size; j++) {
				printf("token %d: (%s)\n", j, pipelines[i]->items[j]);
			}
		}

		if (!pipelines) {
			// split_by_pipe already printed a syntax error if any
			pathSearch(tokens); // call path search function
			free(input);
			free_tokens(tokens);
			continue;
		}

		if (exec_pipeline_filebacked(pipelines) != 0) {
			fprintf(stderr, "pipeline failed\n");
		}

		// Free the split array (remember: it only owns the items arrays + tokenlist structs)
		free_split_tokenlists(pipelines);

		for (int i = 0; i < tokens->size; i++) {
			printf("token %d: (%s)\n", i, tokens->items[i]);
		}

		free(input);
		free_tokens(tokens);
	}

	return 0;
}


