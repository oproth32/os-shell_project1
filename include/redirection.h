#ifndef REDIRECTION_H
#define REDIRECTION_H

#include <stddef.h>
#include <sys/types.h> // pid_t
#include "lexer.h"

// Parsed command after removing redirection operators 
typedef struct {
    char **argv; // NULL-terminated argv (no <, >, filenames) 
    size_t argc; // number of args, not counting the NULL 
    char *in_path; // NULL if no "<" 
    char *out_path; // NULL if no ">" 
} CmdParts;

// Parse < and > out of tokens; build argv. Returns 1 on success, 0 on syntax error. 
int parse_redirection_from_tokens(const tokenlist *tokens, CmdParts *parts);

// Forks; in child applies redirection then execv(path_to_exec, parts->argv).
// Frees parts->argv (the pointer array) in parent. 
void exec_external_with_redir(const char *path_to_exec, CmdParts *parts);
pid_t spawn_external_with_redir(const char *path_to_exec, CmdParts *parts);

#endif // REDIRECTION_H 
