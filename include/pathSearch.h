#ifndef PATHSEARCH_H
#define PATHSEARCH_H

#include "lexer.h"

/* Entry point: resolves command (PATH or direct) and executes (with redirection, via redirection module). */
void pathSearch(tokenlist *tokens);

#endif /* PATHSEARCH_H */
