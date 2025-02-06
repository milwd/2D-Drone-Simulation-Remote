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


void read_json(newBlackboard *bb);
double calculate_score(newBlackboard *bb);
void initialize_logger();
void cleanup_logger();
void create_named_pipe(const char *pipe_name);

int main() {
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
    read_json(bb);
    initialize_logger();

    logger("Blackboard server started...");

    // INITIALIZE THE BLACKBOARD
    bb->state = 0;  // 0 for paused or waiting, 1 for running, 2 for quit
    bb->score = 0.0;
    bb->drone_x = 2;
    bb->drone_y = 2;
    for (int i = 0; i < MAX_OBJECTS; i++) {
        bb->obstacle_xs[i] = -1; bb->obstacle_ys[i] = -1;
        bb->target_xs[i] = -1; bb->target_ys[i] = -1;
    }
    bb->command_force_x = 0; bb->command_force_y = 0;
    bb->max_width   = 20; bb->max_height  = 20; // changes during runtime
    bb->stats.hit_obstacles = 0; bb->stats.hit_targets = 0;
    bb->stats.time_elapsed = 0.0; bb->stats.distance_traveled = 0.0;
    
    while (1) {
        sem_wait(sem);
        bb->score = calculate_score(bb);
        read_json(bb);
        sem_post(sem);
        send_heartbeat(fd);
        sleep(5);  // freq of 0.2 Hz  
    }

    if (fd >= 0) { close(fd); }
    cleanup_logger();
    sem_close(sem);
    sem_unlink(SEM_NAME);
    munmap(bb, sizeof(newBlackboard));
    shm_unlink(SHM_NAME);

    return 0;
}

void read_json(newBlackboard *bb) {
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
