#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "blackboard.h"


void summon(char *args[], int useTerminal);

int main() {

    pid_t allPID[NUMBER_OF_PROCESSES];
    const char *nameOfProcess[NUMBER_OF_PROCESSES] = {
        "Blackboard", "Window", "Dynamics", "Keyboard", "Obstacle", "Target"
    };

    if (access(PIPE_NAME, F_OK) == 0) {
        // FIFO exists, so remove it
        if (unlink(PIPE_NAME) == -1) {
            perror("Failed to remove existing FIFO");
            exit(EXIT_FAILURE);
        }
        printf("Existing FIFO removed.\n");
    }
    if (mkfifo(PIPE_NAME, 0666) == -1) {
        perror("mkfifo failed");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < NUMBER_OF_PROCESSES; i++) {
        pid_t pid = fork();
        allPID[i] = pid;

        if (pid == 0) {  // Child process
            char args[MAX_MSG_LENGTH];
            switch (i) {
                case 0:  // Blackboard process
                    snprintf(args, sizeof(args), "args_for_blackboard");
                    char *argsBlackboard[] = {"./blb.out", args, NULL}; 
                    summon(argsBlackboard, 0);
                    break;
                case 1:  // Window process
                    snprintf(args, sizeof(args), "args_for_window");
                    char *argsWindow[] = {"konsole", "konsole", "-e", "./win.out", args, NULL};
                    summon(argsWindow, 1);
                    break;
                case 2:  // Dynamics process
                    snprintf(args, sizeof(args), "args_for_dynamics");
                    char *argsDynamics[] = {"./dyn.out", args, NULL};
                    summon(argsDynamics, 0);
                    break;
                case 3:  // Keyboard process
                    snprintf(args, sizeof(args), "args_for_keyboard");
                    char *argsKeyboard[] = {"konsole", "konsole", "-e", "./key.out", NULL};
                    summon(argsKeyboard, 1);
                    break;
                case 4:  // Obstacle process
                    snprintf(args, sizeof(args), "args_for_obstacle");
                    char *argsObstacle[] = {"./obs.out", args, NULL};
                    summon(argsObstacle, 0);
                    break;
                case 5:  // Target process
                    snprintf(args, sizeof(args), "args_for_target");
                    char *argsTarget[] = {"./tar.out", args, NULL};
                    summon(argsTarget, 0);
                    break;
            }
            perror("execlp failed");
            exit(EXIT_FAILURE);
        } else if (pid < 0) {
            perror("fork failed");
            exit(EXIT_FAILURE);
        } else {
            printf("Launched %s, PID: %d\n", nameOfProcess[i], pid);
        }
    }

    // Wait for the termination of processes
    int status;
    pid_t terminatedPid = wait(&status);
    if (terminatedPid == -1) {
        perror("waitpid failed");
        exit(EXIT_FAILURE);
    }

    printf("Process %d terminated. Terminating all other processes...\n", terminatedPid);

    // Terminate all other processes
    for (int i = 0; i < NUMBER_OF_PROCESSES; i++) {
        if (allPID[i] != terminatedPid) {
            if (kill(allPID[i], SIGTERM) == -1) {
                perror("kill failed");
                exit(EXIT_FAILURE);
            }
        }
    }

    // Wait for remaining processes to terminate
    while (wait(NULL) > 0);

    // Clean up the named pipe
    unlink(PIPE_NAME);

    printf("All processes terminated. Exiting master process.\n");
    return 0;
}

void summon(char *args[], int useTerminal) {
    if (execvp(args[0], args) == -1) {
        perror("execvp failed");
        exit(EXIT_FAILURE);
    }
    
}
