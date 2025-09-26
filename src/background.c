#include "prompt.h"
#include "lexer.h"
#include "pipeline.h" // split_by_pipe, exec_pipeline_filebacked(_bg)
#include "background.h" // jobs_check_finished, jobs_add, trim_trailing_amp
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/wait.h>

// background job helpers
void jobs_check_finished(Job *jobs, int next_job_id) {
    for (int i = 0; i < MAX_JOBS; i++) if (jobs[i].used) {
        int status;
        pid_t r = waitpid(jobs[i].pid, &status, WNOHANG);
        if (r == jobs[i].pid) {
            printf("[%d] + done %s\n", jobs[i].id, jobs[i].cmdline);
            jobs[i].used = 0;
        }
    }
}

// Add a new background job; returns 0 on success, -1 on error (too many jobs).
int jobs_add(pid_t pid, const char *cmdline, Job *jobs, int next_job_id) {
    for (int i = 0; i < MAX_JOBS; i++) if (!jobs[i].used) {
        jobs[i].used = 1;
        jobs[i].id = next_job_id++;
        jobs[i].pid = pid;
        snprintf(jobs[i].cmdline, sizeof(jobs[i].cmdline), "%s", cmdline);
        printf("[%d] %d\n", jobs[i].id, pid);
        return 0;
    }
    fprintf(stderr, "Too many background jobs (max %d)\n", MAX_JOBS);
    return -1;
}

// Trim spaces and one trailing '&' for display purposes.
/* Helper: trim trailing whitespace and a final '&' (with optional spaces before it)
   from a display string used in the jobs table. Safe in-place. */
static void trim_trailing_amp(char *s) {
    if (!s) return;
    size_t len = strlen(s);
    // trim trailing whitespace
    while (len > 0 && (s[len-1] == ' ' || s[len-1] == '\t' || s[len-1] == '\n'))
        s[--len] = '\0';
    // if now ends with '&', remove it
    if (len > 0 && s[len-1] == '&') {
        s[--len] = '\0';
        // trim any whitespace that was before the '&'
        while (len > 0 && (s[len-1] == ' ' || s[len-1] == '\t'))
            s[--len] = '\0';
    }
}
