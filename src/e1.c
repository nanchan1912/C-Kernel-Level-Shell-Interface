#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/fcntl.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include "a3.h"

#include "e1.h"  // Include its own header to ensure correct declarations

// Global variables to store the parsed tokens and their count.
// These are defined here and declared as extern in a3.h


// Struct to hold information about a background job.
// Definition should be in a header or only one .c file.
// The extern declaration in e1.h is sufficient for other files.
struct Job jobs[100];
int job_count = 0;

// A global variable to track the foreground process PID.
// It is marked volatile because it is modified by a signal handler.
volatile pid_t foreground_pid = -1;

// Signal handler for SIGINT (Ctrl-C).
void sigint_handler(int sig) {
    if (foreground_pid > 0) {
        kill(foreground_pid, SIGINT);
    }
}

// Signal handler for SIGTSTP (Ctrl-Z).
void sigtstp_handler(int sig) {
    if (foreground_pid > 0) {
        kill(foreground_pid, SIGTSTP);
    }
}

// E.1: The activities command implementation.
// Comparison function for qsort to sort jobs lexicographically by command name.
int compare_jobs(const void* a, const void* b) {
    const struct Job* jobA = (const struct Job*)a;
    const struct Job* jobB = (const struct Job*)b; // Corrected line
    return strcmp(jobA->command, jobB->command);
}

void execute_activities() {
    // Check for completed processes to update their status.
    for (int i = 0; i < job_count; ++i) {
        if (jobs[i].active) {
            int status;
            pid_t result = waitpid(jobs[i].pid, &status, WNOHANG);
            if (result == jobs[i].pid) {
                jobs[i].active = 0; // Mark as completed
            }
        }
    }

    // Filter out inactive jobs and copy active ones to a temporary array.
    struct Job active_jobs[100];
    int active_count = 0;
    for (int i = 0; i < job_count; ++i) {
        if (jobs[i].active) {
            active_jobs[active_count] = jobs[i];
            active_count++;
        }
    }

    // Sort the active jobs lexicographically by command name.
    qsort(active_jobs, active_count, sizeof(struct Job), compare_jobs);

    // Print the sorted list of active jobs.
    for (int i = 0; i < active_count; ++i) {
        int status;
        // Use waitpid with WNOHANG and WUNTRACED to check for stopped processes.
        pid_t result = waitpid(active_jobs[i].pid, &status, WNOHANG | WUNTRACED);
        const char* state = "Running";
        if (result == active_jobs[i].pid && WIFSTOPPED(status)) {
            state = "Stopped";
        }
        printf("[%d] : %s - %s\n", active_jobs[i].pid, active_jobs[i].command, state);
    }
}

// E.2: The ping command implementation.
void execute_ping(pid_t pid, int signal_number) {
    int actual_signal = signal_number % 32;
    if (kill(pid, actual_signal) == 0) {
        printf("Sent signal %d to process with pid %d\n", actual_signal, pid);
    } else {
        if (errno == ESRCH) {
            fprintf(stderr, "No such process found\n");
        } else {
            perror("ping failed");
        }
    }
}

// E.4: The fg command implementation.
void execute_fg(int job_number) {
    int job_index = -1;
    if (job_number == -1) { // No job number provided, find the most recent active job.
        for (int i = job_count - 1; i >= 0; i--) {
            if (jobs[i].active) {
                job_index = i;
                break;
            }
        }
    } else { // A specific job number was provided.
        if (job_number > 0 && job_number <= job_count && jobs[job_number - 1].active) {
            job_index = job_number - 1;
        }
    }

    if (job_index != -1) {
        printf("%s\n", jobs[job_index].command);
        if (kill(jobs[job_index].pid, SIGCONT) < 0) {
            perror("Failed to send SIGCONT");
        }
        
        int status;
        if (waitpid(jobs[job_index].pid, &status, WUNTRACED) > 0) {
            if (WIFSTOPPED(status)) {
                printf("\nJob with pid %d stopped.\n", jobs[job_index].pid);
            } else {
                jobs[job_index].active = 0;
            }
        }
    } else {
        fprintf(stderr, "No such job\n");
    }
}

// E.4: The bg command implementation.
void execute_bg(int job_number) {
    if (job_number > 0 && job_number <= job_count) {
        int job_index = job_number - 1;
        if (jobs[job_index].active) {
            int status;
            pid_t result = waitpid(jobs[job_index].pid, &status, WNOHANG | WUNTRACED);
            if (result == jobs[job_index].pid && WIFSTOPPED(status)) {
                if (kill(jobs[job_index].pid, SIGCONT) == 0) {
                    printf("[%d] %s &\n", job_number, jobs[job_index].command);
                } else {
                    perror("Failed to send SIGCONT");
                }
            } else {
                fprintf(stderr, "Job already running\n");
            }
        } else {
            fprintf(stderr, "No such job\n");
        }
    } else {
        fprintf(stderr, "No such job\n");
    }
}


// C.1: Command Execution.
// This function executes a single command with redirected I/O.
void execute_single_command(char** argv, int input_fd, int output_fd) {
    if (input_fd != STDIN_FILENO) {
        if (dup2(input_fd, STDIN_FILENO) < 0) {
            perror("dup2 input failed");
        }
        close(input_fd);
    }
    if (output_fd != STDOUT_FILENO) {
        if (dup2(output_fd, STDOUT_FILENO) < 0) {
            perror("dup2 output failed");
        }
        close(output_fd);
    }
    execvp(argv[0], argv);
    // If execvp returns, it means an error occurred.
    if (errno == ENOENT) {
        fprintf(stderr, "Command not found!\n");
    }
    //perror("Command execution failed");
    exit(1);
}

// C.2, C.3, C.4: File Redirection and Pipes.
// This function parses and executes a full pipeline. It returns the PID of the last process.
pid_t execute_pipeline(int start_pos, int end_pos, int is_background) {
    int current_pos = start_pos;
    int prev_pipe_read_fd = STDIN_FILENO; // Start with standard input
    pid_t last_pid = -1;

    while (current_pos < end_pos) {
        int command_start = current_pos;
        int next_pipe_pos = end_pos;

        // Find the end of the current command or the next pipe.
        for (int i = command_start; i < end_pos; ++i) {
            if (strcmp(tokens[i], "|") == 0) {
                next_pipe_pos = i;
                break;
            }
        }

        char* argv[100];
        int arg_count = 0;
        char* input_file = NULL;
        char* output_file = NULL;
        int append_mode = 0;

        // Parse command arguments and I/O redirection files.
        for (int i = command_start; i < next_pipe_pos; ++i) {
            if (strcmp(tokens[i], "<") == 0) {
                if (i + 1 < next_pipe_pos) {
                    input_file = tokens[i+1];
                    i++;
                }
            } else if (strcmp(tokens[i], ">") == 0) {
                if (i + 1 < next_pipe_pos) {
                    output_file = tokens[i+1];
                    append_mode = 0;
                    i++;
                }
            } else if (strcmp(tokens[i], ">>") == 0) {
                if (i + 1 < next_pipe_pos) {
                    output_file = tokens[i+1];
                    append_mode = 1;
                    i++;
                }
            } else {
                argv[arg_count++] = tokens[i];
            }
        }
        argv[arg_count] = NULL;

        if (arg_count == 0) {
            current_pos = next_pipe_pos + 1;
            continue;
        }

        // C.4: Pipes - create a pipe for the current command if needed
        int pipe_fd[2] = {-1, -1};
        if (next_pipe_pos < end_pos) {
            if (pipe(pipe_fd) < 0) {
                perror("Pipe creation failed");
                exit(1);
            }
        }
        
        pid_t pid = fork();
        if (pid == -1) {
            perror("Fork failed");
            exit(1);
        } else if (pid == 0) { // Child process
            if (is_background) {
                // Background processes must not have access to the terminal for input.
                int devnull = open("/dev/null", O_RDONLY);
                if (devnull < 0) {
                    perror("Failed to open /dev/null");
                }
                dup2(devnull, STDIN_FILENO);
                close(devnull);
            }

            // Close the previous pipe's read end if it exists.
            if (prev_pipe_read_fd != STDIN_FILENO) {
                dup2(prev_pipe_read_fd, STDIN_FILENO);
                close(prev_pipe_read_fd);
            }
            
            // Close the write end of the current pipe in the child
            if (pipe_fd[0] != -1) {
                close(pipe_fd[0]);
            }
            
            // C.3: Output Redirection
            int output_fd = STDOUT_FILENO;
            if (output_file != NULL) {
                if (pipe_fd[1] != -1) {
                    close(pipe_fd[1]); // Close the write end of the pipe as file redirection takes precedence
                }
                if (append_mode) {
                    output_fd = open(output_file, O_WRONLY | O_CREAT | O_APPEND, 0666);
                } else {
                    output_fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0666);
                }
            } else if (pipe_fd[1] != -1) {
                output_fd = pipe_fd[1];
            }

            // C.2: Input Redirection
            int input_fd = STDIN_FILENO;
            if (input_file != NULL) {
                input_fd = open(input_file, O_RDONLY);
                if (input_fd < 0) {
                    fprintf(stderr, "No such file or directory!\n");
                    exit(1);
                }
            }

            execute_single_command(argv, input_fd, output_fd);

        } else { // Parent process
            last_pid = pid;
            
            // The parent needs to close its read end of the previous pipe
            if (prev_pipe_read_fd != STDIN_FILENO) {
                close(prev_pipe_read_fd);
            }
            
            // The parent needs to close its write end of the new pipe
            if (pipe_fd[1] != -1) {
                close(pipe_fd[1]);
            }
            // The parent's read end of the new pipe becomes the input for the next command
            prev_pipe_read_fd = pipe_fd[0];
        }
        current_pos = next_pipe_pos + 1;
    }

    // The parent must close the final pipe read end after the loop finishes.
    if (prev_pipe_read_fd != STDIN_FILENO) {
        close(prev_pipe_read_fd);
    }

    return last_pid;
}