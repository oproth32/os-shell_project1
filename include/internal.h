#ifndef INTERNAL_H
#define INTERNAL_H

#include "lexer.h"
#include "background.h"

int run_internal(tokenlist *tokens, Job *jobs, int next_job_id);
void add_history(const char *line);
void print_history(void);

#endif // INTERNAL_H 