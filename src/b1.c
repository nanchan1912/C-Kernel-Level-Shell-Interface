#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/wait.h>
#include<sys/fcntl.h>
#include<pwd.h>
#include<string.h>
#include<dirent.h>
#include "b1.h"

// Global variables to track directories
char home_dir[1000];
char last_dir[1000] = "";
char current_dir[1000];

// Global flags for reveal
int l_nigu = 0;
int a_nigu = 0;

// Function declarations
void hop_home();
void hop_previous();
void hop_parent();
void hop_name(char* name);
int compare_strings(const void* a, const void* b);
void set_l_a(char* str);
void list_dir(char *path);

// Hop Command Functions
void hop_home(){
    char temp_dir[1000];
    if (getcwd(temp_dir, sizeof(temp_dir)) == NULL) {
        perror("getcwd");
        return;
    }
    
    if (chdir(getenv("HOME")) != 0) {
        perror("chdir");
    } else {
        strcpy(last_dir, temp_dir);
    }
}

void hop_previous(){
    char temp_dir[1000];
    if (getcwd(temp_dir, sizeof(temp_dir)) == NULL) {
        perror("getcwd");
        return;
    }

    if (strcmp(last_dir, "") == 0) {
        return;
    }
    
    if (chdir(last_dir) == 0) {
        strcpy(last_dir, temp_dir);
    } else {
        perror("chdir");
    }
}

void hop_parent(){
    char temp_dir[1000];
    if (getcwd(temp_dir, sizeof(temp_dir)) == NULL) {
        perror("getcwd");
        return;
    }
    
    if (chdir("..") != 0) {
        perror("chdir");
    } else {
        strcpy(last_dir, temp_dir);
    }
}

void hop_name(char* name){
    char temp_dir[1000];
    if (getcwd(temp_dir, sizeof(temp_dir)) == NULL) {
        fprintf(stderr, "No such directory!\n");
        return;
    }
    
    if(chdir(name) == 0){
        strcpy(last_dir, temp_dir);
    } else {
        fprintf(stderr, "No such directory!\n");
    }
}

void hop_runner(char* input){
    char* hop_token = strtok(input, " \t\n");
    hop_token = strtok(NULL, " \t\n");

    while (hop_token != NULL) {
        if (strcmp(hop_token, "~") == 0) {
            hop_home();
        } else if (strcmp(hop_token, ".") == 0) {
            // do nothing
        } else if (strcmp(hop_token, "..") == 0) {
            hop_parent();
        } else if (strcmp(hop_token, "-") == 0) {
            hop_previous();
        } else {
            hop_name(hop_token);
        }
        hop_token = strtok(NULL, " \t\n");
    }
}

// Reveal Command Functions
int compare_strings(const void* a, const void* b) {
    const char** sa = (const char**)a;
    const char** sb = (const char**)b;
    return strcasecmp(*sa, *sb);
}

void set_l_a(char* str){
    int n = strlen(str);
    for(int i = 0; i < n; i++){
        if(str[i] == 'a'){
            a_nigu = 1;
        }
        if(str[i] == 'l'){
            l_nigu = 1;
        }
    }
}

void list_dir(char *path) {
    DIR *dir = opendir(path);
    if (!dir) {
        fprintf(stderr, "No such directory!\n");
        return;
    }

    struct dirent *entry;
    char **names = NULL;
    int count = 0;
    
    while ((entry = readdir(dir)) != NULL) {
        if (!a_nigu && entry->d_name[0] == '.') {
            continue;
        }
        names = realloc(names, sizeof(char*) * (count + 1));
        if (names == NULL) {
            perror("realloc");
            closedir(dir);
            return;
        }
        names[count] = strdup(entry->d_name);
        if (names[count] == NULL) {
            perror("strdup");
            for (int i = 0; i < count; i++) {
                free(names[i]);
            }
            free(names);
            closedir(dir);
            return;
        }
        count++;
    }
    closedir(dir);

    qsort(names, count, sizeof(char*), compare_strings);

    for (int i = 0; i < count; i++) {
        if (l_nigu){
            printf("%s\n", names[i]);
        } else {
            printf("%s ", names[i]);
        }
        free(names[i]);
    }
    free(names);
    if (!l_nigu) printf("\n");
}
void reveal_runner(char* input) {
    a_nigu = 0;
    l_nigu = 0;
    char *path_to_list = ".";
    char raw_copy[1000];
    strcpy(raw_copy, input);

    char *reveal_token = strtok(raw_copy, " \t\n");
    
    int path_count = 0; // Counter for non-flag arguments
    
    // First, check for reveal - case as we discussed previously
    reveal_token = strtok(NULL, " \t\n");
    if (reveal_token != NULL && strcmp(reveal_token, "-") == 0) {
        if (strtok(NULL, " \t\n") == NULL) {
            fprintf(stderr, "No such directory!\n");
            return;
        }
    }
    
    // Re-parse the input to count non-flag arguments properly
    strcpy(raw_copy, input);
    reveal_token = strtok(raw_copy, " \t\n");
    reveal_token = strtok(NULL, " \t\n");
    
    while(reveal_token != NULL) {
        if(reveal_token[0] != '-') {
            path_count++;
            path_to_list = reveal_token; // Keep track of the last path provided
        }
        reveal_token = strtok(NULL, " \t\n");
    }
    
    // If more than one non-flag argument, it's a syntax error
    if (path_count > 1) {
        fprintf(stderr, "reveal: Invalid Syntax!\n"); // Or "Invalid Syntax!" depending on what your overall test suite expects
        return;
    }

    // Now, run the command with the correct flags and path
    // Re-tokenize the input one last time to get the correct path and flags
    a_nigu = 0;
    l_nigu = 0;
    path_to_list = ".";
    strcpy(raw_copy, input);
    reveal_token = strtok(raw_copy, " \t\n");
    reveal_token = strtok(NULL, " \t\n");
    
    while(reveal_token != NULL) {
        if(reveal_token[0] == '-') {
            set_l_a(reveal_token + 1);
        } else {
            path_to_list = reveal_token;
        }
        reveal_token = strtok(NULL, " \t\n");
    }

    list_dir(path_to_list);
}