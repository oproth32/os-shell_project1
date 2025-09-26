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
		// if the token starts with a ~, replace it with the value of the HOME environment variable
		else if (tok[0] == '~' && (tok[1] == '\\' || tok[1] == '\0'))
		{
			char *home = getenv("HOME");
			if (home != NULL)
			{

				size_t homelen = strlen(home);
				size_t tolen = strlen(&tok[1]);
				char *newtok = (char *)malloc(homelen + tolen + 1);
				if (newtok)
				{
					memcpy(newtok, home, homelen);
					memcpy(&newtok[homelen], &tok[1], tolen + 1);
					newtok[homelen + tolen] = 0;
					add_token(tokens, newtok);
					free(newtok);
				}
				else
					add_token(tokens, tok); // if malloc fails, just add the original token
			}
			else
			{
				add_token(tokens, tok);
			}
			tok = strtok(NULL, " ");
			continue;

		}
		add_token(tokens, tok);
		tok = strtok(NULL, " ");
	}
}