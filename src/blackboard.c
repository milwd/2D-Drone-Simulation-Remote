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


void read_parameters(int *num_obstacles, int *num_targets, int *mass, int *visc_damp_coef, int *obst_repl_coef, int *radius);
double calculate_score(newBlackboard *bb);
void initialize_logger();
void cleanup_logger();

int main() {
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
    sem_t *sem = sem_open(SEM_NAME, O_CREAT, 0666, 0);
    if (sem == SEM_FAILED) {
        perror("sem_open failed");
        return 1;
    }
    initialize_logger();

    bb->score = 0.0;
    bb->drone_x = 2;
    bb->drone_y = 2;
    bb->n_obstacles = 5;
    bb->n_targets = 5;
    for (int i = 0; i < 99; i++) {
        bb->obstacle_xs[i] = -1;
        bb->obstacle_ys[i] = -1;
        bb->target_xs[i] = -1;
        bb->target_ys[i] = -1;
    }
    bb->command_force_x = 0; 
    bb->command_force_y = 0;
    bb->max_width   = 20;
    bb->max_height  = 20;

    bb->stats.hit_obstacles = 0;
    bb->stats.hit_targets = 0;
    bb->stats.time_elapsed = 0.0;
    bb->stats.distance_traveled = 0.0;

    bb->state = 0;  // 0 for paused or waiting, 1 for running, 2 for quit

    logger("Blackboard server started...");


    int n_targets, n_obstacles;
    while (1) {
        sem_wait(sem);

        bb->score = calculate_score(bb);
        
        read_parameters(&n_obstacles, &n_targets, &mass, &visc_damp_coef, &obst_repl_coef, &radius);
        if (bb->n_obstacles != n_obstacles) {
            bb->n_obstacles = n_obstacles;
        }
        if (bb->n_targets != n_targets) {
            bb->n_targets = n_targets;
        }

        sem_post(sem);
        sleep(5);  // freq of 0.2 Hz  
    }

    cleanup_logger();
    sem_close(sem);
    sem_unlink(SEM_NAME);
    munmap(bb, sizeof(newBlackboard));
    shm_unlink(SHM_NAME);

    return 0;
}

void read_parameters(int *num_obstacles, int *num_targets, int *mass, int *visc_damp_coef, int *obst_repl_coef, int *radius) {
    FILE *file = fopen(PARAM_FILE, "r");
    if (file == NULL) {
        perror("Failed to open parameter file");
        return;
    }
    char line[256];
    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "num_obstacles=", 14) == 0) {
            *num_obstacles = atoi(line + 14);
        } else if (strncmp(line, "num_targets=", 12) == 0) {
            *num_targets = atoi(line + 12);
        } else if (strncmp(line, "mass=", 5) == 0) {
            *mass = atoi(line + 5);
        } else if (strncmp(line, "visc_damp_coef=", 15) == 0) {
            *visc_damp_coef = atoi(line + 15);
        } else if (strncmp(line, "obst_repl_coef=", 15) == 0) {
            *obst_repl_coef = atoi(line + 15);
        } else if (strncmp(line, "radius=", 7) == 0) {
            *radius = atoi(line + 7);
        } 
    }
    fclose(file);
}

double calculate_score(newBlackboard *bb) {
    double score = (double)bb->stats.hit_targets        * 10.0 - 
                   (double)bb->stats.hit_obstacles      * 5.0 - 
                   bb->stats.time_elapsed               * 0.1 - 
                   bb->stats.distance_traveled          * 0.05;
    return score;
}

void initialize_logger() {
    if (access("simulation.log", F_OK) == 0) {
        if (unlink("simulation.log") == 0) {
            printf("Existing simulation.log file deleted successfully.\n");
        } else {
            perror("Failed to delete simulation.log");
        }
    } else {
        printf("No existing simulation.log file found.\n");
    }
    if (!log_file) {
        log_file = fopen("simulation.log", "a");
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

