#ifndef PIPELINE_H
#define PIPELINE_H

#include "redirection.h"
#include "lexer.h"

// Split tokens by '|' into an array of tokenlist* (NULL-terminated).
tokenlist** split_by_pipe(const tokenlist *tokens);
// Execute a pipeline of commands connected by pipes, using temporary files for intermediate stages.
int exec_pipeline_filebacked(tokenlist **segments);
// Free the array returned by split_by_pipe (frees tokenlist structs and their items arrays).
void free_split_tokenlists(tokenlist **lists);
// Execute a pipeline in background using temporary files between stages.
pid_t exec_pipeline_filebacked_bg(tokenlist **segments);

#endif /* PIPELINE_H */
