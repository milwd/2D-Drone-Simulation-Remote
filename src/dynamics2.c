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
#include <stdbool.h>

#include "blackboard.h"
#include "logger.h"


void compute_repulsive_force(double *Fx, double *Fy, newBlackboard *bb);
void compute_attractive_force(double *Fx, double *Fy, newBlackboard *bb);


int main() {
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
    // bb->drone_x = 0;
    // bb->drone_y = 0;
    double x_i = bb->drone_x, x_i_minus_1 = bb->drone_x, x_i_minus_2 = bb->drone_x;
    double y_i = bb->drone_y, y_i_minus_1 = bb->drone_y, y_i_minus_2 = bb->drone_y;

    while (1){
        sem_wait(sem);

        double Fx = bb->command_force_x;
        double Fy = bb->command_force_y;
        double repulsive_Fx = 0.0, repulsive_Fy = 0.0;
        double attractive_Fx = 0.0, attractive_Fy = 0.0;
        bb->obstacle_xs[0] = 20;
        bb->obstacle_ys[0] = 20;
        compute_repulsive_force(&repulsive_Fx, &repulsive_Fy, bb);
        // compute_attractive_force(&attractive_Fx, &attractive_Fy, bb);
        Fx += repulsive_Fx + attractive_Fx;
        printf("repulsive_Fx: %f, repulsive_Fy: %f, command force: %f, pos x: %d, y: %d\n", repulsive_Fx, repulsive_Fy, Fx, bb->drone_x, bb->drone_y);
        Fy += repulsive_Fy + attractive_Fy;

        double x_i_new = (1/(mass+visc_damp_coef*DT)) * (Fx * DT * DT - mass * (x_i_minus_1-2*x_i) + visc_damp_coef * DT * x_i);
        double y_i_new = (1/(mass+visc_damp_coef*DT)) * (Fy * DT * DT - mass * (y_i_minus_1-2*y_i) + visc_damp_coef * DT * y_i);

        bb->drone_x = x_i_new;
        bb->drone_y = y_i_new;

        x_i_minus_2 = x_i_minus_1;  //TODO WHAT THE FUCK IS Xi-2
        x_i_minus_1 = x_i;
        x_i = x_i_new;

        y_i_minus_2 = y_i_minus_1;
        y_i_minus_1 = y_i;
        y_i = y_i_new;

        if (bb->drone_x < 1) { bb->drone_x = 1; vx = 0; Fx = 0;}
        if (bb->drone_x > bb->max_width-1) { bb->drone_x = bb->max_width-2; vx = 0; Fx = 0;}
        if (bb->drone_y < 1) { bb->drone_y = 1; vy = 0; Fy = 0;} 
        if (bb->drone_y > bb->max_height-1) { bb->drone_y = bb->max_height-2; vy = 0; Fy = 0;}

        for (int i=0; i<bb->n_obstacles; i++){  // TODO FIX THIS AND MAYBE MERGE
            if (bb->drone_x == bb->obstacle_xs[i] && bb->drone_y == bb->obstacle_ys[i]){
                bb->stats.hit_obstacles += 1;
                bb->obstacle_xs[i] = -1;
                bb->obstacle_ys[i] = -1;
                logger("Drone hit an obstacle at position (%d, %d)", bb->drone_x, bb->drone_y);
            } 
        }
        for (int i=0; i<bb->n_targets; i++){
            if (bb->drone_x == bb->target_xs[i] && bb->drone_y == bb->target_ys[i]){
                bb->stats.hit_targets += 1;
                bb->target_xs[i] = -1;
                bb->target_ys[i] = -1;
                logger("Drone got a target at position (%d, %d)", bb->drone_x, bb->drone_y);
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
    *Fx = 0; *Fy = 0;

    for (int i = 0; i < bb->n_obstacles; i++) {
        double dx = bb->obstacle_xs[i] - bb->drone_x;
        double dy = bb->obstacle_ys[i] - bb->drone_y;
        double dist = sqrt(dx * dx + dy * dy);
        if (dist < radius && dist > 0) {
            double repulsive = obst_repl_coef * (1.0 / dist - 1.0 / radius) / (dist * dist + EPSILON);
            *Fx -= repulsive * (dx / (dist + EPSILON));
            *Fy -= repulsive * (dy / (dist + EPSILON));
        }
    } // Obstacles

    if (bb->drone_x < radius) {
        double repulsive = obst_repl_coef * (1.0 / (bb->drone_x + EPSILON) - 1.0 / radius) / (bb->drone_x * bb->drone_x + EPSILON);
        *Fx += repulsive;
    } // Left wall
    // if (bb->max_width - bb->drone_x < radius) {
    //     double dist = bb->max_width - bb->drone_x + EPSILON;
    //     double repulsive = 2 / (dist);
    //     *Fx -= repulsive;
    // } // Right wall
    // if (bb->drone_y < radius) {
    //     double dist = bb->drone_y + EPSILON;
    //     double repulsive = 2 / (dist);
    //     *Fy += repulsive;
    // } // Top wall
    // if (bb->max_height - bb->drone_y < radius) {
    //     double dist = bb->max_height - bb->drone_y + EPSILON;
    //     double repulsive = 2 / (dist);
    //     *Fy -= repulsive;
    // } // Bottom wall
    if (*Fx > 100){ *Fx = 100;}
    if (*Fy > 100){ *Fy = 100;}
    if (*Fx < -100){ *Fx = -100;}
    if (*Fy < -100){ *Fy = -100;}
}

void compute_attractive_force(double *Fx, double *Fy, newBlackboard *bb) {
    *Fx = 0;
    *Fy = 0;

    for (int i = 0; i < bb->n_targets; i++) {
        double dx = bb->target_xs[i] - bb->drone_x;
        double dy = bb->target_ys[i] - bb->drone_y;
        double dist = sqrt(dx * dx + dy * dy);

        if (dist < radius && dist > 0) {
            double attractive = obst_repl_coef * (1.0 / dist - 1.0 / radius) / (dist * dist + EPSILON);
            *Fx += attractive * (dx / (dist + EPSILON));
            *Fy += attractive * (dy / (dist + EPSILON));
        }
    }
}
