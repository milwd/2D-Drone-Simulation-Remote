#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>
#include "blackboard.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <semaphore.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdbool.h>
#include "blackboard.h"
#include <sys/stat.h>
#include <cjson/cJSON.h>


void summon(char *args[], int useTerminal);
double calculate_score(newBlackboard *bb);
void initialize_logger();
void cleanup_logger();
void read_json(newBlackboard *bb, bool first_time);
void create_named_pipe(const char *pipe_name);
void handle_sigchld(int sig);

bool terminated = false;

int main() {
    signal(SIGCHLD, handle_sigchld);  

    create_named_pipe(PIPE_BLACKBOARD);
    create_named_pipe(PIPE_DYNAMICS);
    create_named_pipe(PIPE_KEYBOARD);
    create_named_pipe(PIPE_WINDOW);
    create_named_pipe(PIPE_OBSTACLE);
    create_named_pipe(PIPE_TARGET);
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open failed");
        return 1;
    }
    if (ftruncate(shm_fd, sizeof(newBlackboard)) == -1) {
        perror("ftruncate failed");
        return 1;
    }
    newBlackboard *bb = mmap(NULL, sizeof(newBlackboard), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (bb == MAP_FAILED) {
        perror("mmap failed");
        return 1;
    }
    memset(bb, 0, sizeof(newBlackboard));
    sem_t *sem = sem_open(SEM_NAME, O_CREAT, 0666, 0); // works like a mutex TODO check
    if (sem == SEM_FAILED) {
        perror("sem_open failed");
        return 1;
    }
    int fd = open_watchdog_pipe(PIPE_BLACKBOARD);   
    read_json(bb, true);
        
    initialize_logger();
    logger("Blackboard server started. PID: %d", getpid());

    // INITIALIZE THE BLACKBOARD
    bb->state = 0;  // 0 for paused or waiting, 1 for running, 2 for quit
    bb->score = 0.0;
    bb->drone_x = 2; bb->drone_y = 2;
    for (int i = 0; i < MAX_OBJECTS; i++) {
        bb->obstacle_xs[i] = -1; bb->obstacle_ys[i] = -1;
        bb->target_xs[i] = -1; bb->target_ys[i] = -1;
    }
    bb->command_force_x = 0; bb->command_force_y = 0;
    bb->max_width   = 20; bb->max_height  = 20; // changes during runtime
    bb->stats.hit_obstacles = 0; bb->stats.hit_targets = 0;
    bb->stats.time_elapsed = 0.0; bb->stats.distance_traveled = 0.0;

    bb->score = calculate_score(bb);
    read_json(bb, true);

    int mode;
    while (1) {
        printf("\n=== === === ===\n\nWELCOME TO DRONE SIMULATION.\n\nChoose mode of operation ...\n"
            "(1): Local object generation and simulation\n"
            "(2): Remote object generation and simulation\n"
            "Enter mode: ");
        if (scanf("%d", &mode) != 1) {
            fprintf(stderr, "Invalid input. Please enter a number (1 or 2).\n");
            while (getchar() != '\n'); // Clear input buffer
            continue;
        }
        if (mode == 1 || mode == 2) {
            break;
        }
        fprintf(stderr, "Invalid choice. Please enter 1 or 2.\n");
    }
    printf("\n=== === === ===\n\n");

    pid_t allPIDs[NUMBER_OF_PROCESSES] = {0};
    int processCount = (mode == 2) ? NUMBER_OF_PROCESSES - 1 : NUMBER_OF_PROCESSES;
    const char *processNames[NUMBER_OF_PROCESSES];
    if (mode == 1) {
        const char *temp[] = {"Window", "Dynamics", "Keyboard", "Watchdog", "Obstacle", "Target"};
        memcpy(processNames, temp, sizeof(temp));
    } else if (mode == 2) {
        const char *temp[] = {"Window", "Dynamics", "Keyboard", "Watchdog", "ObjectSub"};
        memcpy(processNames, temp, sizeof(temp));
    }
    
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

    while (1) {
        if (terminated) {
            break;
        }
        sem_wait(sem);
        bb->score = calculate_score(bb);
        read_json(bb, true);
        sem_post(sem);
        send_heartbeat(fd);
        sleep(BLACKBOARD_CHECK_DELAY);  // freq of 0.2 Hz  
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
    if (fd >= 0) { close(fd); }  // close pipe
    cleanup_logger();
    sem_close(sem);
    sem_unlink(SEM_NAME);
    munmap(bb, sizeof(newBlackboard));
    shm_unlink(SHM_NAME);

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

void read_json(newBlackboard *bb, bool first_time) {
    const char *filename = JSON_PATH;
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Could not open file");
    }
    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);
    char *data = (char *)malloc(length + 1);
    if (!data) {
        perror("Memory allocation failed for JSON config");
        fclose(file);
    }
    fread(data, 1, length, file);
    data[length] = '\0';
    fclose(file);
    cJSON *json = cJSON_Parse(data);
    if (!json) {
        printf("Error parsing JSON: %s\n", cJSON_GetErrorPtr());
    }    
    cJSON *item;  // TODO CHECK FOR NULL and CORRUPTED JSON
    if ((item = cJSON_GetObjectItem(json, "num_obstacles")))    bb->n_obstacles = item->valueint;
    if ((item = cJSON_GetObjectItem(json, "num_targets")))      bb->n_targets = item->valueint;
    if ((item = cJSON_GetObjectItem(json, "mass")))             bb->physix.mass = item->valueint;
    if ((item = cJSON_GetObjectItem(json, "visc_damp_coef")))   bb->physix.visc_damp_coef = item->valueint;
    if ((item = cJSON_GetObjectItem(json, "obst_repl_coef")))   bb->physix.obst_repl_coef = item->valueint;
    if ((item = cJSON_GetObjectItem(json, "radius")))           bb->physix.radius = item->valueint;
    cJSON_Delete(json);
    free(data);
}


double calculate_score(newBlackboard *bb) {
    double score = (double)bb->stats.hit_targets        * 30.0 - 
                   (double)bb->stats.hit_obstacles      * 5.0 - 
                   bb->stats.time_elapsed               * 0.05 - 
                   bb->stats.distance_traveled          * 0.1;
    return score;
}

void initialize_logger() {
    if (access("./logs/simulation.log", F_OK) == 0) {
        if (unlink("./logs/simulation.log") == 0) {
            printf("Existing simulation.log file deleted successfully.\n");
        } else {
            perror("Failed to delete simulation.log");
        }
    } else {
        printf("No existing simulation.log file found.\n");
    }
    if (!log_file) {
        log_file = fopen("./logs/simulation.log", "a");
        if (!log_file) {
            perror("Unable to open log file");
            return;
        }
    }
}

void cleanup_logger() {
    pthread_mutex_lock(&logger_mutex);
    if (log_file) {
        fclose(log_file);
        log_file = NULL;
    }
    pthread_mutex_unlock(&logger_mutex);
}

void create_named_pipe(const char *pipe_name) {
    if (access(pipe_name, F_OK) == -1) {
        if (mkfifo(pipe_name, 0666) == -1) {
            perror("Failed to create named pipe");
            exit(EXIT_FAILURE);
        }
    }
}

void handle_sigchld(int sig) {
    terminated = true;
}