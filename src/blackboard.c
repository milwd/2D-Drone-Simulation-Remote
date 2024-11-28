#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <semaphore.h>
#include <unistd.h>
#include <sys/wait.h>
#include "blackboard.h"

#define NUM_CHILDREN 2  // Window and Keyboard binaries

void read_parameters(int *num_obstacles, int *num_targets);
double calculate_score(newBlackboard *bb);

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

    // Set up semaphore
    sem_t *sem = sem_open(SEM_NAME, O_CREAT, 0666, 1);
    if (sem == SEM_FAILED) {
        perror("sem_open failed");
        return 1;
    }

    // Initialize shared memory structure
    bb->score = 0.0;
    bb->drone_x = 0;
    bb->drone_y = 0;
    bb->n_obstacles = 5;
    bb->n_targets = 5;
    for (int i = 0; i < 99; i++) {
        bb->obstacle_xs[i] = -1;
        bb->obstacle_ys[i] = -1;
        bb->target_xs[i] = -1;
        bb->target_ys[i] = -1;
    }

    bb->stats.hit_obstacles = 0;
    bb->stats.hit_targets = 0;
    bb->stats.time_elapsed = 0.0;
    bb->stats.distance_traveled = 0.0;

    printf("Blackboard server started...\n");


    int n_targets, n_obstacles;
    while (1) {
        sem_wait(sem);

        bb->score = calculate_score(bb);
        
        // Read parameters
        read_parameters(&n_obstacles, &n_targets);
        if (bb->n_obstacles != n_obstacles) {
            bb->n_obstacles = n_obstacles;
        }
        if (bb->n_targets != n_targets) {
            bb->n_targets = n_targets;
        }

        sem_post(sem);
        sleep(5);  // sleep for 5 seconds  
    }

    sem_close(sem);
    sem_unlink(SEM_NAME);
    munmap(bb, sizeof(newBlackboard));
    shm_unlink(SHM_NAME);

    return 0;
}

void read_parameters(int *num_obstacles, int *num_targets) {
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



