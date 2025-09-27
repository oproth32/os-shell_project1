#include "background.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>

/* ---------------- init ---------------- */
void jobs_init(Job jobs[]) {
    for (int i = 0; i < MAX_JOBS; ++i) {
        jobs[i].used = 0;
        jobs[i].id   = 0;
        jobs[i].pid  = -1;
        jobs[i].cmdline[0] = '\0';
    }
}

/* ---------------- add ---------------- */
int jobs_add(pid_t pid, const char *cmdline, Job jobs[], int *next_job_id) {
    if (pid <= 0 || !cmdline || !*cmdline || !jobs || !next_job_id) {
        return -1;
    }
    /* find free slot */
    for (int i = 0; i < MAX_JOBS; ++i) {
        if (!jobs[i].used) {
            jobs[i].used = 1;
            jobs[i].id   = *next_job_id;   /* use current id */
            jobs[i].pid  = pid;
            snprintf(jobs[i].cmdline, sizeof(jobs[i].cmdline), "%s", cmdline);

            /* print start line */
            printf("[%d] %d\n", jobs[i].id, (int)pid);
            fflush(stdout);

            /* advance id (never reused) */
            (*next_job_id)++;
            return 0;
        }
    }
    fprintf(stderr, "Too many background jobs (max %d)\n", MAX_JOBS);
    return -1;
}

/* ---------------- reap/check ---------------- */
/* Reap ANY finished child; if it's the tracked pid for a job, announce + free. */
void jobs_check_finished(Job jobs[], int next_job_id) {
    (void)next_job_id; /* not needed here, but kept to match your existing prototypes */

    int status;
    pid_t done;
    /* Reap as many as finished to prevent zombies. */
    while ((done = waitpid(-1, &status, WNOHANG)) > 0) {
        /* Find if this pid is the tracked "last stage" for any job */
        int found = 0;
        for (int i = 0; i < MAX_JOBS; ++i) {
            if (jobs[i].used && jobs[i].pid == done) {
                printf("[%d] + done %s\n", jobs[i].id, jobs[i].cmdline);
                fflush(stdout);
                jobs[i].used = 0;
                found = 1;
                break;
            }
        }
        /* If not found, it's likely an earlier pipeline stage; ignore (but reaped). */
    }
    /* done < 0:
       - if errno == ECHILD, no children; otherwise transient error — ignore. */
}

/* ---------------- list (for Part 9; harmless now) ---------------- */
void jobs_list(const Job jobs[]) {
    int any = 0;
    for (int i = 0; i < MAX_JOBS; ++i) {
        if (jobs[i].used) {
            any = 1;
            printf("[%d]+ %d %s\n", jobs[i].id, (int)jobs[i].pid, jobs[i].cmdline);
        }
    }
    if (!any) {
        printf("No active background processes.\n");
    }
}

/* ---------------- util ---------------- */
void trim_trailing_amp(char *s) {
    if (!s) return;
    size_t len = strlen(s);
    /* trim trailing whitespace */
    while (len > 0 && (s[len-1] == ' ' || s[len-1] == '\t' || s[len-1] == '\n'))
        s[--len] = '\0';
    /* remove final '&' if present */
    if (len > 0 && s[len-1] == '&') {
        s[--len] = '\0';
        while (len > 0 && (s[len-1] == ' ' || s[len-1] == '\t'))
            s[--len] = '\0';
    }
}
