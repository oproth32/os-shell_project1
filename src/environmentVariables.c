#include "environmentVariables.h"
#include <stdlib.h>
#include <string.h>
#include <lexer.h>

void change_env_var(char *tok, tokenlist *tokens) {
    // iterate through each token
	while (tok != NULL)
	{
		// if the token contains a newline character, replace it with a null terminator
		for (int i = 0; i < strlen(tok); i++)
		{
			if (tok[i] == '\n')
			{
				tok[i] = 0;
				break;
			}
		}
		// if the token starts with a $, replace it with the value of the environment variable
		if (tok[0] == '$')
		{
			char *env = getenv(&tok[1]);
			if (env != NULL)
				tok = env;
			else
				tok = "";
		}
		add_token(tokens, tok);
		tok = strtok(NULL, " ");
	}
}