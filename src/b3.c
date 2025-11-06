#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/wait.h>
#include<sys/fcntl.h>
#include<pwd.h>
#include<string.h>
#include<dirent.h>
#include "b1.h"
#include "a3.h"

#include "e1.h"
// we start with the fact that we have gotten the input ( some random command )
// now log all the commands
char history[15][1024];  // store up to 15 commands
int history_count = 0;    // number of commands currently stored

void load_history() {
    int fd = open("logs.txt", O_RDONLY | O_CREAT, 0644);
    if(fd < 0) return;

    char buf[1024];
    int pos = 0;
    int count = 0;

    int bytes_read;
    while((bytes_read = read(fd, buf + pos, 1)) > 0){
        if(buf[pos] == '\n'){
            buf[pos] = '\0';
            if(count < 15){
                strcpy(history[count], buf);
                count++;
            }
            pos = 0; // reset for next line
        } else {
            pos++;
        }
    }

    history_count = count;
    close(fd);
}

void save_history() {
    int fd = open("logs.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if(fd < 0) return;

    for(int i = 0; i < history_count; i++){
        write(fd, history[i], strlen(history[i]));
        write(fd, "\n", 1);
    }

    close(fd);
}

void log_command(char* input){
    // Do not log if the command starts with "log".
    // This uses strncmp to check for "log" at the beginning of the string.
    if (strncmp(input, "log", 3) == 0 && (input[3] == ' ' || input[3] == '\t' || input[3] == '\n' || input[3] == '\0')) {
        return;
    }
    
    // Do not log if identical to last command.
    if(history_count > 0 && strcmp(history[history_count-1], input) == 0){
        return;
    }

    // If already 15 commands, shift older ones.
    if(history_count == 15){
        for(int i = 1; i < 15; i++){
            strcpy(history[i-1], history[i]);
        }
        history_count--; // Will add new command at end.
    }

    strcpy(history[history_count], input);
    history_count++;

    save_history(); // Save updated array to file.
}

void execute_index(int i){
    if(i < 1 || i > history_count){
        printf("Not stored, index out of bounds\n");
        return;
    }

    // newest command is last element
    int idx = i - 1;
    char command_to_do[1024];
    strcpy(command_to_do, history[idx]);

    // check first word
    char command_copy[1024];
    strcpy(command_copy, command_to_do);
    char *first_word = strtok(command_copy, " \t\n");

    if(strcmp(first_word, "hop") == 0){
        hop_runner(command_to_do);
    }
    else if(strcmp(first_word, "reveal") == 0){
        reveal_runner(command_to_do);
    }
    else{
        printf("Cannot execute: %s\n", command_to_do);
    }
}

void purge_logs() {
    history_count = 0;
    int fd = open("logs.txt", O_WRONLY | O_TRUNC);
    if(fd >= 0){
        close(fd);
        printf("Log history cleared.\n");
    }
}

void log_runner(char* input) {
    //load_history();
    char raw_copy[1024];
    strcpy(raw_copy, input);   // keep original before strtok messes it up

    char* log_token = strtok(input, " \t\n");

    if (log_token == NULL) return;

    // if first guy aint log then take in the entire thing
    if (strcmp(log_token, "log") != 0) {
        log_command(raw_copy);   // use untouched copy
        return;
    }

    // Now handle "log" subcommands
    log_token = strtok(NULL, " \t\n");
    if (log_token == NULL) {
        // just "log" → print history
        for (int i = 0; i < history_count; i++) {
            printf("%s\n", history[i]); // newest = 1
        }
        return;
    }

    if (strcmp(log_token, "purge") == 0) {
        purge_logs();
        return;
    }
    else if (strcmp(log_token, "execute") == 0) {
        char *index_token = strtok(NULL, " \t\n");
        if (index_token == NULL) {
            printf("Usage: log execute <index>\n");
            return;
        }
        int idx = atoi(index_token);
        execute_index(idx);
    }
    else {
        printf("Unknown log command: %s\n", log_token);
    }
}

