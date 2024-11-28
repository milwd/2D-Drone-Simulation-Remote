#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <math.h>
#include "blackboard.h"
#include "logger.h"


void compute_repulsive_force(double *Fx, double *Fy, newBlackboard *bb);
void compute_attractive_force(double *Fx, double *Fy, newBlackboard *bb);


int main() {
    // int shmid = shmget(SHM_KEY, sizeof(newBlackboard), 0666);
    // if (shmid == -1) {
    //     perror("shmget failed");
    //     return 1;
    // }
    // newBlackboard *bb = (newBlackboard *)shmat(shmid, NULL, 0);
    // if (bb == (void *)-1) {
    //     perror("shmat failed");
    //     return 1;
    // }
    int shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open failed");
        return 1;
    }
    newBlackboard *bb = mmap(NULL, sizeof(newBlackboard), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (bb == MAP_FAILED) {
        perror("mmap failed");
        return 1;
    }
    sem_t *sem = sem_open(SEM_NAME, 0);
    if (sem == SEM_FAILED) {
        perror("sem_open failed");
        return 1;
    }

    double vx = 0, vy = 0;
    bb->drone_x = 0;
    bb->drone_y = 0;
    double x_i = bb->drone_x, x_i_minus_1 = bb->drone_x, x_i_minus_2 = bb->drone_x;
    double y_i = bb->drone_y, y_i_minus_1 = bb->drone_y, y_i_minus_2 = bb->drone_y;

    while (1){
        sem_wait(sem);

        double Fx = bb->command_force_x;
        double Fy = bb->command_force_y;
        double repulsive_Fx = 0.0, repulsive_Fy = 0.0;
        double attractive_Fx = 0.0, attractive_Fy = 0.0;
        compute_repulsive_force(&repulsive_Fx, &repulsive_Fy, bb);
        compute_attractive_force(&attractive_Fx, &attractive_Fy, bb);
        Fx += repulsive_Fx + attractive_Fx;
        Fy += repulsive_Fy + attractive_Fy;

        double x_i_new = (Fx * DT * DT / M) - (K * (x_i - x_i_minus_1) * DT / M) + (2 * x_i - x_i_minus_1);
        double y_i_new = (Fy * DT * DT / M) - (K * (y_i - y_i_minus_1) * DT / M) + (2 * y_i - y_i_minus_1);

        bb->drone_x = x_i_new;
        bb->drone_y = y_i_new;

        x_i_minus_2 = x_i_minus_1;
        x_i_minus_1 = x_i;
        x_i = x_i_new;

        y_i_minus_2 = y_i_minus_1;
        y_i_minus_1 = y_i;
        y_i = y_i_new;

        // printf("Fx: %.2f, Fy: %.2f\n", Fx, Fy);

        if (bb->drone_x < 1) { bb->drone_x = 1; vx = 0; }
        if (bb->drone_x > WIN_SIZE_X-1) { bb->drone_x = WIN_SIZE_X-1; vx = 0; }
        if (bb->drone_y < 1) { bb->drone_y = 1; vy = 0; } 
        if (bb->drone_y > WIN_SIZE_Y-1) { bb->drone_y = WIN_SIZE_Y-1; vy = 0; }

        for (int i=0; i<bb->n_obstacles; i++){
            if (bb->drone_x == bb->obstacle_xs[i] && bb->drone_y == bb->obstacle_ys[i]){
                bb->stats.hit_obstacles += 1;
                // log_message("Drone hit an obstacle at position (%d, %d)", bb->drone_x, bb->drone_y);
            }
        }
        for (int i=0; i<bb->n_targets; i++){
            if (bb->drone_x == bb->target_xs[i] && bb->drone_y == bb->target_ys[i]){
                bb->stats.hit_targets += 1;
                bb->target_xs[i] = -1;
                bb->target_ys[i] = -1;
                // log_message("Drone got a target at position (%d, %d)", bb->drone_x, bb->drone_y);
            }
        }
        if (x_i != x_i_minus_1 || y_i != y_i_minus_1){
            bb->stats.distance_traveled += sqrt((x_i - x_i_minus_1) * (x_i - x_i_minus_1) + (y_i - y_i_minus_1) * (y_i - y_i_minus_1));
        }

        sem_post(sem);
        usleep(DT * 1000000);
    }
    
    munmap(bb, sizeof(newBlackboard));
    return 0;
}


void compute_repulsive_force(double *Fx, double *Fy, newBlackboard *bb) {
    *Fx = 0;
    *Fy = 0;

    const double epsilon = 1e-6; // Small value to avoid division by zero

    for (int i = 0; i < bb->n_obstacles; i++) {
        double dx = bb->obstacle_xs[i] - bb->drone_x;
        double dy = bb->obstacle_ys[i] - bb->drone_y;
        double dist = sqrt(dx * dx + dy * dy);

        if (dist < R && dist > 0) {
            double repulsive = ETA * (1.0 / dist - 1.0 / R) / (dist * dist + epsilon);
            *Fx -= repulsive * (dx / dist);
            *Fy -= repulsive * (dy / dist);
        }
    } // Obstacles

    // if (bb->drone_x < R) {
    //     double dist = bb->drone_x + epsilon;
    //     double repulsive = 0.000001 / (dist * dist);
    //     *Fx += repulsive;
    // } // Left wall
    // if (WIN_SIZE_X - bb->drone_x < R) {
    //     double dist = WIN_SIZE_X - bb->drone_x + epsilon;
    //     double repulsive = 0.000001 / (dist * dist);
    //     *Fx -= repulsive;
    // } // Right wall
    // if (bb->drone_y < R) {
    //     double dist = bb->drone_y + epsilon;
    //     double repulsive = 0.000001 / (dist * dist);
    //     *Fy += repulsive;
    // } // Top wall
    // if (WIN_SIZE_Y - bb->drone_y < R) {
    //     double dist = WIN_SIZE_Y - bb->drone_y + epsilon;
    //     double repulsive = 0.000001 / (dist * dist);
    //     *Fy -= repulsive;
    // } // Bottom wall
}

void compute_attractive_force(double *Fx, double *Fy, newBlackboard *bb) {
    *Fx = 0;
    *Fy = 0;

    for (int i = 0; i < bb->n_targets; i++) {
        double dx = bb->target_xs[i] - bb->drone_x;
        double dy = bb->target_ys[i] - bb->drone_y;
        double dist = sqrt(dx * dx + dy * dy);

        if (dist < R && dist > 0) {
            double attractive = ETA * (1.0 / dist - 1.0 / R) / (dist * dist);
            *Fx += attractive * (dx / dist);
            *Fy += attractive * (dy / dist);
        }
    }
}
