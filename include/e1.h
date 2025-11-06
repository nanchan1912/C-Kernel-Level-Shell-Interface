#ifndef E1_H
#define E1_H

// This file contains function prototypes for shell core functionalities.

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/fcntl.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>

// Struct definition here
struct Job {
    pid_t pid;
    char command[100];
    int active;
};

// Extern declarations for globals defined in e1.c
extern struct Job jobs[100];
extern int job_count;
extern volatile pid_t foreground_pid;

// Function prototypes
void sigint_handler(int sig);
void sigtstp_handler(int sig);
int compare_jobs(const void* a, const void* b);
void execute_activities();
void execute_ping(pid_t pid, int signal_number);
void execute_fg(int job_number);
void execute_bg(int job_number);
void execute_single_command(char** argv, int input_fd, int output_fd);
pid_t execute_pipeline(int start_pos, int end_pos, int is_background);

#endif // E1_H