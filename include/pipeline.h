#ifndef PIPELINE_H
#define PIPELINE_H

#include "lexer.h"
#include "redirection.h"
#include <sys/types.h> // pid_t
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif



typedef struct {
    CmdParts parts;             // argv/in/out from redirection parser
    char     exec_path[PATH_MAX];
    pid_t    pid;
} Stage;


/* Split tokens on '|' into an array of tokenlist* (NULL-terminated).
   Each tokenlist* borrows token pointers from the original list.
   Return NULL on syntax error or OOM. */
tokenlist **split_by_pipe(const tokenlist *tokens);

/* Free the array returned by split_by_pipe (frees each tokenlist's items array
   and the tokenlist structs, then the outer array). Does NOT free the strings. */
void free_split_tokenlists(tokenlist **lists);

/* Execute a pipeline in the foreground (blocks until all stages exit).
   Returns 0 on success (all children created; actual exit codes not surfaced),
   or -1 on immediate failure to set up/launch the pipeline. */
int exec_pipeline(tokenlist **segments);

/* Execute a pipeline in the background (does NOT wait).
   Returns the PID of the LAST stage (to be used as the job PID), or -1 on failure. */
pid_t exec_pipeline_bg(tokenlist **segments);

#endif /* PIPELINE_H */
