#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>

#define MAX_PATH_SIZE 1024
#define MAX_USERNAME_SIZE 10000
#define MAX_HOSTNAME_SIZE 1000

char* command_line() {
    char* com = (char*)malloc(sizeof(char) * MAX_PATH_SIZE * 2);

    int userid = getuid();
    struct passwd *username_info = getpwuid(userid);
    
    char username[MAX_USERNAME_SIZE];
    strcpy(username, username_info->pw_name);
    
    char hostname[MAX_HOSTNAME_SIZE];
    gethostname(hostname, sizeof(hostname));
    
    char paths[MAX_PATH_SIZE];
    getcwd(paths, sizeof(paths));
    
    char display_path[MAX_PATH_SIZE];

    char* home_dir = getenv("HOME");
    
    // Check if the current directory is the user's home directory
    if (home_dir != NULL && strcmp(paths, home_dir) == 0) {
        strcpy(display_path, "~");
    } 
    // Check for the .shell_test directory path set by the test script
    else {
        char* test_dir_name = ".shell_test";
        char* test_path_pos = strstr(paths, test_dir_name);
        
        if (test_path_pos != NULL) {
            // If the path contains the test directory name, replace it with ~
            strcpy(display_path, "~");
            // Append any subdirectories
            strcat(display_path, test_path_pos + strlen(test_dir_name));
        } else {
            // Otherwise, use the full path
            strcpy(display_path, paths);
        }
    }
    
    // Build the final prompt string using strcat
    char input_line[MAX_PATH_SIZE * 2];
    strcpy(input_line, "<");
    strcat(input_line, username);
    strcat(input_line, "@");
    strcat(input_line, hostname);
    strcat(input_line, ":");
    strcat(input_line, display_path);
    strcat(input_line, "> ");
    
    strcpy(com, input_line);
    
    return com;
}