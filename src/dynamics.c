#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include "blackboard.h"


void compute_repulsive_force(double *Fx, double *Fy, newBlackboard *bb);
void compute_attractive_force(double *Fx, double *Fy, newBlackboard *bb);


int main() {
    // Access shared memory
    int shmid = shmget(SHM_KEY, sizeof(newBlackboard), 0666);
    if (shmid == -1) {
        perror("shmget failed");
        return 1;
    }

    newBlackboard *bb = (newBlackboard *)shmat(shmid, NULL, 0);
    if (bb == (void *)-1) {
        perror("shmat failed");
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
        double repulsive_Fx = 0, repulsive_Fy = 0;
        double attractive_Fx = 0, attractive_Fy = 0;
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

        printf("Fx: %.2f, Fy: %.2f\n", Fx, Fy);

        if (bb->drone_x < 0) { bb->drone_x = 0; vx = 0; }
        if (bb->drone_x > WIN_SIZE_X) { bb->drone_x = WIN_SIZE_X; vx = 0; }
        if (bb->drone_y < 0) { bb->drone_y = 0; vy = 0; }
        if (bb->drone_y > WIN_SIZE_Y) { bb->drone_y = WIN_SIZE_Y; vy = 0; }

        printf("Drone position: (%d, %d)\n", bb->drone_x, bb->drone_y);

        sem_post(sem);
        usleep(DT * 1000000);
    }

    shmdt(bb);

    return 0;
}


void compute_repulsive_force(double *Fx, double *Fy, newBlackboard *bb) {
    *Fx = 0;
    *Fy = 0;

    for (int i = 0; i < bb->n_obstacles; i++) {
        double dx = bb->obstacle_xs[i] - bb->drone_x;
        double dy = bb->obstacle_ys[i] - bb->drone_y;
        double dist = sqrt(dx * dx + dy * dy);

        if (dist < R && dist > 0) {
            double repulsive = ETA * (1.0 / dist - 1.0 / R) / (dist * dist);
            *Fx -= repulsive * (dx / dist);
            *Fy -= repulsive * (dy / dist);
        }
    }
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
