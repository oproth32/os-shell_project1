#ifndef BACKGROUND_H
#define BACKGROUND_H

#include <sys/types.h>
#include "background.h"

#define MAX_JOBS 10

typedef struct {
    int used;
    int id; // job number (1,2,3,...) never reused
    pid_t pid; // pid of single cmd OR last stage of pipeline
    char cmdline[1024]; // for "[job] + done <cmdline>"
} Job;

void jobs_check_finished(Job *jobs, int next_job_id);
int jobs_add(pid_t pid, const char *cmdline, Job *jobs, int next_job_id);
void trim_trailing_amp(char *s);

#endif