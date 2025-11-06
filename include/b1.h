#ifndef B1_H
#define B1_H

// This file contains function prototypes for hop, reveal, and log functionalities.

#include <unistd.h>
#include <sys/wait.h>
#include <sys/fcntl.h>
#include <string.h>
#include <stdio.h>

extern char history[15][1024];
extern int history_count;
extern int l_nigu;
extern int a_nigu;
extern char home_dir[1000];
extern char last_dir[1000];
extern char current_dir[1000];

void load_history();
void save_history();
void log_command(char* input);
void execute_index(int i);
void purge_logs();
void log_runner(char* input);
void hop_home();
void hop_previous();
void hop_parent();
void hop_name(char* name);
void hop_runner(char* input);
void set_l_a(char* str);
void list_dir(char *path);
void reveal_runner(char* input);
char* command_line();

#endif // B1_H
