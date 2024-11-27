#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <fcntl.h>
#include <semaphore.h>
#include <unistd.h>
#include "blackboard.h"


void read_parameters(int *num_obstacles, int *num_targets);
double calculate_score(newBlackboard *bb);

int main() {
    int shmid = shmget(SHM_KEY, sizeof(newBlackboard), IPC_CREAT | 0666);
    if (shmid == -1) { 
        perror("shmget failed");
        return 1;
    }
    newBlackboard *bb = (newBlackboard *)shmat(shmid, NULL, 0);
    if (bb == (newBlackboard *)-1) {
        perror("shmat failed");
        return 1;
    }
    sem_t *sem = sem_open(SEM_NAME, O_CREAT, 0666, 1);
    if (sem == SEM_FAILED) {
        perror("sem_open failed");
        return 1;
    }

    bb->score = 0.0;
    bb->drone_x = 0;
    bb->drone_y = 0;
    bb->n_obstacles = 5;
    bb->n_targets = 5;
    bb->n_obstacles = 5;
    for (int i=0; i<99; i++){
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
        
        read_parameters(&n_obstacles, &n_targets);
        if (bb->n_obstacles != n_obstacles){
            bb->n_obstacles = n_obstacles;
        }
        if (bb->n_targets != n_targets){
            bb->n_targets = n_targets;
        }
        sem_post(sem);
        sleep(5);  // sleep for 5 seconds  
    }

    sem_close(sem);
    sem_unlink(SEM_NAME);
    shmdt(bb);
    shmctl(shmid, IPC_RMID, NULL);

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