#include "prompt.h"
#include "lexer.h"
#include "pipeline.h"   // split_by_pipe, exec_pipeline_filebacked(_bg)
#include "background.h" // jobs_check_finished, jobs_add, trim_trailing_amp
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/wait.h>

/* -------- background job helpers -------- */
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

/* Trim spaces and one trailing '&' for display purposes. */
void trim_trailing_amp(char *s) {
    if (!s) return;
    size_t len = strlen(s);
    while (len > 0 && isspace((unsigned char)s[len - 1])) s[--len] = '\0';
    if (len > 0 && s[len - 1] == '&') {
        s[--len] = '\0';
        while (len > 0 && isspace((unsigned char)s[len - 1])) s[--len] = '\0';
    }
}