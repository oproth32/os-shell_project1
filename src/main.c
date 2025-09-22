#include "lexer.h"
#include "prompt.h"
#include "environmentVariables.h"
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
		for (int i = 0; i < tokens->size; i++) {
			printf("token %d: (%s)\n", i, tokens->items[i]);
		}

		free(input);
		free_tokens(tokens);
	}

	return 0;
}

