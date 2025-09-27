#ifndef BACKGROUND_H
#define BACKGROUND_H

#include <sys/types.h>

#define MAX_JOBS 10

// Background job structure
typedef struct {
    int used;
    int id; // job number (1,2,3,...) never reused
    pid_t pid; // pid of single cmd OR last stage of pipeline
    char cmdline[1024]; // for "[job] + done <cmdline>"
} Job;

// Initialize jobs array
void jobs_init(Job jobs[]);

/* Add a new background job; prints "[id] pid" on success.
   Returns 0 on success, -1 if table full. Increments *next_job_id on success. */
int jobs_add(pid_t pid, const char *cmdline, Job jobs[], int *next_job_id);

/* Reap finished children (waitpid(-1, WNOHANG)) and, when a tracked pid exits,
   print "[id] + done <cmdline>" and free its slot. */
void jobs_check_finished(Job jobs[], int next_job_id);

/* For built-in 'jobs' later: print active background jobs (optional for Part 8). */
void jobs_list(const Job jobs[]);

/* Utility: trim trailing whitespace and a trailing '&' (in-place). */
void trim_trailing_amp(char *s);

#endif /* BACKGROUND_H */