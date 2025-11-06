#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>

#include "a3.h"
#include "b1.h"
#include "e1.h"
#include "name.h"
#include "b3.h"

#define MAX_INPUT_SIZE 1024

int main() {
    char input_buffer[MAX_INPUT_SIZE];
    
    // Initialize history and signal handlers.
    load_history();
    signal(SIGINT, sigint_handler);
    signal(SIGTSTP, sigtstp_handler);

    // Change to the user's home directory at startup to match test requirements
    

    while (1) {
        // Check for completed background processes.
        for (int i = 0; i < job_count; ++i) {
            if (jobs[i].active) {
                int status;
                pid_t result = waitpid(jobs[i].pid, &status, WNOHANG);
                if (result == jobs[i].pid) {
                    printf("[%d] %s with pid %d exited %s\n", i + 1, jobs[i].command, jobs[i].pid,
                           WIFEXITED(status) && WEXITSTATUS(status) == 0 ? "normally" : "abnormally");
                    jobs[i].active = 0;
                }
            }
        }
        
        char* prompt = command_line();
        printf("%s", prompt);
        fflush(stdout);
        free(prompt);

        if (fgets(input_buffer, sizeof(input_buffer), stdin) == NULL) {
            printf("logout\n");
            for (int i = 0; i < job_count; ++i) {
                if (jobs[i].active) {
                    kill(jobs[i].pid, SIGKILL);
                }
            }
            break;
        }

        input_buffer[strcspn(input_buffer, "\n")] = 0;
        
        if (strlen(input_buffer) == 0) {
            continue;
        }

        // Create a copy of the input for logging.
        char log_input_copy[MAX_INPUT_SIZE];
        strcpy(log_input_copy, input_buffer);
        log_command(log_input_copy);

        // Tokenize the input.
        char tokenize_copy[MAX_INPUT_SIZE];
        strcpy(tokenize_copy, input_buffer);
        part_part(tokenize_copy);
        
        if (token_count == 0) {
            continue;
        }
        
        // Validate syntax.
        pos = 0;
        int isValid = shell_cmd();

        if (isValid && pos == token_count) {
            // Commands that require a fresh copy of the input string for parsing.
            
            if (strcmp(tokens[0], "hop") == 0) {
                char hop_copy[MAX_INPUT_SIZE];
                strcpy(hop_copy, input_buffer);
                hop_runner(hop_copy);
                continue;
            }
            if (strcmp(tokens[0], "reveal") == 0) {
                char reveal_copy[MAX_INPUT_SIZE];
                strcpy(reveal_copy, input_buffer);
                reveal_runner(reveal_copy);
                continue;
            }
            if (strcmp(tokens[0], "log") == 0) {
                char log_copy[MAX_INPUT_SIZE];
                strcpy(log_copy, input_buffer);
                log_runner(log_copy);
                continue;
            }

            // Existing logic for other commands.
            if (strcmp(tokens[0], "activities") == 0 && token_count == 1) {
                execute_activities();
                continue;
            }
            if (strcmp(tokens[0], "ping") == 0 && token_count == 3) {
                pid_t pid = atoi(tokens[1]);
                int signal_num = atoi(tokens[2]);
                execute_ping(pid, signal_num);
                continue;
            }
            if (strcmp(tokens[0], "fg") == 0) {
                int job_number = -1;
                if (token_count == 2) {
                    job_number = atoi(tokens[1]);
                } else if (token_count > 2) {
                    fprintf(stderr, "fg: usage: fg [job_number]\n");
                    continue;
                }
                execute_fg(job_number);
                continue;
            }
            if (strcmp(tokens[0], "bg") == 0) {
                if (token_count != 2) {
                    fprintf(stderr, "bg: usage: bg job_number\n");
                    continue;
                }
                int job_number = atoi(tokens[1]);
                execute_bg(job_number);
                continue;
            }
            
            // Handle sequential and background execution for external commands.
            int current_pos = 0;
            while (current_pos < token_count) {
                int end_of_group_pos = token_count;
                int is_background = 0;
                char cmd_string[100] = "";

                for (int i = current_pos; i < token_count; ++i) {
                    if (strlen(cmd_string) + strlen(tokens[i]) + 1 < sizeof(cmd_string)) {
                        strcat(cmd_string, tokens[i]);
                        strcat(cmd_string, " ");
                    } else {
                        break;
                    }
                    
                    if (strcmp(tokens[i], ";") == 0) {
                        end_of_group_pos = i;
                        break;
                    } else if (strcmp(tokens[i], "&") == 0) {
                        end_of_group_pos = i;
                        is_background = 1;
                        break;
                    }
                }
                
                if (is_background) {
                    pid_t last_pid = execute_pipeline(current_pos, end_of_group_pos, 1);
                    if (last_pid > 0 && job_count < 100) {
                        jobs[job_count].pid = last_pid;
                        strncpy(jobs[job_count].command, cmd_string, sizeof(jobs[job_count].command) - 1);
                        jobs[job_count].command[sizeof(jobs[job_count].command) - 1] = '\0';
                        jobs[job_count].active = 1;
                        printf("[%d] %d\n", job_count + 1, last_pid);
                        job_count++;
                    } else {
                        fprintf(stderr, "Failed to start background job or job list is full\n");
                    }
                } else {
                    pid_t last_pid = execute_pipeline(current_pos, end_of_group_pos, 0);
                    int status;
                    
                    foreground_pid = last_pid;
                    if (last_pid > 0) {
                        waitpid(last_pid, &status, WUNTRACED);
                    }
                    
                    if (WIFSTOPPED(status)) {
                        if (job_count < 100) {
                            jobs[job_count].pid = last_pid;
                            strncpy(jobs[job_count].command, cmd_string, sizeof(jobs[job_count].command) - 1);
                            jobs[job_count].command[sizeof(jobs[job_count].command) - 1] = '\0';
                            jobs[job_count].active = 1;
                            printf("[%d] Stopped %s\n", job_count + 1, jobs[job_count].command);
                            job_count++;
                        } else {
                            fprintf(stderr, "Failed to add stopped job, job list is full\n");
                        }
                    }
                    foreground_pid = -1;
                }
                
                current_pos = end_of_group_pos + 1;
            }
        } else {
            fprintf(stderr, "Invalid Syntax!\n");
        }
    }
    
    save_history();
    return 0;
}
