#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>
#include "blackboard.h"


void summon(char *args[], int useTerminal);

int main() {
    int mode;
    printf("\n=== === === ===\n\nWELCOME TO DRONE SIMULATION.\n\nChoose mode of operation ...\n"
           "(1): Local object generation and simulation\n"
           "(2): Remote object generation and simulation\n"
           "(3): Object generation and publish over network\n"
           "Enter mode: ");
    
    if (scanf("%d", &mode) != 1) {
        fprintf(stderr, "Invalid input. Exiting.\n");
        return EXIT_FAILURE;
    }
    printf("\n=== === === ===\n\n");

    pid_t allPIDs[NUMBER_OF_PROCESSES] = {0};
    int processCount = (mode == 2) ? NUMBER_OF_PROCESSES - 1 : NUMBER_OF_PROCESSES;
    const char *processNames[NUMBER_OF_PROCESSES];
    if (mode == 1) {
        const char *temp[] = {"Blackboard", "Window", "Dynamics", "Keyboard", "Watchdog", "Obstacle", "Target"};
        memcpy(processNames, temp, sizeof(temp));
    } else if (mode == 2) {
        const char *temp[] = {"Blackboard", "Window", "Dynamics", "Keyboard", "Watchdog", "ObjectSub"};
        memcpy(processNames, temp, sizeof(temp));
    }
    
    if (mode == 3) {  
        // Mode 3: Object Publisher
        pid_t pid = fork();
        if (pid == 0) {  
            // Child process
            char *args[] = {"./bins/ObjectPub.out", "args_for_object", NULL};
            summon(args, 0);
        } else if (pid < 0) {
            perror("fork failed");
            return EXIT_FAILURE;
        } else {
            printf("Launched Object Publisher, PID: %d\n", pid); 
        }
    } else {
        for (int i = 0; i < processCount; i++) {
            if (i == 0){
                sleep(1);  
            }
            pid_t pid = fork();
            if (pid == 0) {  
                // Child process
                char args[256];
                snprintf(args, sizeof(args), "args_for_%s", processNames[i]);
                
                char *execArgs[] = {NULL, args, NULL};
                if (strcmp(processNames[i], "Window") == 0 || strcmp(processNames[i], "Keyboard") == 0) {
                    execArgs[0] = "konsole"; 
                    execArgs[1] = "-e";
                    execArgs[2] = (strcmp(processNames[i], "Window") == 0) ? "./bins/Window.out" : "./bins/Keyboard.out";
                    execArgs[3] = NULL;
                } else {
                    snprintf(args, sizeof(args), "args_for_%s", processNames[i]);
                    execArgs[0] = (char*)malloc(strlen(processNames[i]) + 5);
                    sprintf(execArgs[0], "./bins/%s.out", processNames[i]);
                    if (strcmp(processNames[i], "Blackboard") == 0) {
                    } 
                }

                summon(execArgs, (execArgs[0] == "konsole"));

                free(execArgs[0]);  
                exit(EXIT_FAILURE); 
            } else if (pid < 0) {
                perror("fork failed");
                return EXIT_FAILURE;
            } else {
                allPIDs[i] = pid;
                printf("Launched %s, PID: %d\n", processNames[i], pid);  // TODO DELETE LATER
            }
        }
    }

    // Wait for any process to terminate
    int status;
    pid_t terminatedPid = wait(&status);
    if (terminatedPid == -1) {
        perror("wait failed");
        return EXIT_FAILURE;
    }

    printf("Process %d terminated. Terminating all other processes...\n", terminatedPid);

    // Terminate remaining processes
    for (int i = 0; i < processCount; i++) {
        if (allPIDs[i] != 0 && allPIDs[i] != terminatedPid) {
            kill(allPIDs[i], SIGTERM);
        }
    }

    // Wait for all processes to finish
    while (wait(NULL) > 0);

    printf("All processes terminated. Exiting master process.\n");
    return EXIT_SUCCESS;
}

void summon(char *args[], int useTerminal) {
    if (execvp(args[0], args) == -1) {
        perror("execvp failed");
        exit(EXIT_FAILURE);
    }
}
