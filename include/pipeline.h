#ifndef PIPELINE_H
#define PIPELINE_H

#include "redirection.h"
#include "lexer.h"

tokenlist** split_by_pipe(const tokenlist *tokens);
int exec_pipeline_filebacked(tokenlist **segments);
void free_split_tokenlists(tokenlist **lists);
pid_t exec_pipeline_filebacked_bg(tokenlist **segments);

#endif /* PIPELINE_H */
