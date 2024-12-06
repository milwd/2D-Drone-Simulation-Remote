#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>
#include "blackboard.h"


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
    srand((unsigned int)time(NULL));

    int gen_x, gen_y;
    while (1) {
        sem_wait(sem);

        for (int i=0; i<99; i++){
            bb->obstacle_xs[i] = -1;
            bb->obstacle_ys[i] = -1;
        }

        for (int i=0; i<bb->n_obstacles; i++){ 
            gen_x = rand() % (WIN_SIZE_X-1);
            gen_y = rand() % (WIN_SIZE_Y-1);
            while (gen_x == bb->drone_x && gen_y == bb->drone_y){
                gen_x = rand() % (WIN_SIZE_X-1);
                gen_y = rand() % (WIN_SIZE_Y-1);
            }
            bb->obstacle_xs[i] = gen_x;
            bb->obstacle_ys[i] = gen_y;
        }

        sem_post(sem);
        sleep(4);
    }

    sem_close(sem);
    // shmdt(bb);
    munmap(bb, sizeof(newBlackboard));
    return 0;
}
