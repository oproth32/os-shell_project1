#ifndef ENVIRONMENTVARIABLES_H
#define ENVIRONMENTVARIABLES_H

#include "lexer.h"

// Replace environment variables (e.g., $VAR) and ~ in the given token string.
// Adds the resulting tokens to the provided tokenlist.
void change_env_var(char *tok, tokenlist *tokens);
#endif