#ifndef A3_H
#define A3_H

#include <string.h>
#include <unistd.h>
#include <stdio.h>

extern char tokens[1000][1000];
extern int token_count;
extern int pos;

int spacer(char c);
int special(char *s);
void part_part(char* input);
char* current_token(int pos);
int name(char* token);
int input(int pos);
int output(int pos);
int atomic_grp();
int cmd_group();
int shell_cmd();

#endif // A3_H