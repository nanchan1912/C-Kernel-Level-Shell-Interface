#ifndef B3_H
#define B3_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/fcntl.h>
#include <pwd.h>
#include <string.h>

// Global variable declarations (defined in b3.c)
extern char history[15][1024];
extern int history_count;

// Function prototypes
void load_history();
void save_history();
void log_command(char* input);
void execute_index(int i);
void purge_logs();
void log_runner(char* input);
char* command_line();

#endif // B3_H