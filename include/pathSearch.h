#ifndef PATHSEARCH_H
#define PATHSEARCH_H

#include "lexer.h"

// Entry point: resolves command (PATH or direct) and executes (with redirection, via redirection module).
int pathSearch(tokenlist *tokens, int fg_bg, pid_t *out_pid);
int resolve_command(const char *cmd, char *out_path, size_t out_sz);

#endif // PATHSEARCH_H
