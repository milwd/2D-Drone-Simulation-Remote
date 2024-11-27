#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <unistd.h>
#include "blackboard.h"


int main() {
    
    int shmid = shmget(SHM_KEY, sizeof(newBlackboard), 0666);
    if (shmid == -1) {
        perror("shmget failed");
        return 1;
    }

    newBlackboard *bb = (newBlackboard *)shmat(shmid, NULL, 0);
    if (bb == (newBlackboard *)-1) {
        perror("shmat failed");
        return 1;
    }

    sem_t *sem = sem_open(SEM_NAME, 0);
    if (sem == SEM_FAILED) {
        perror("sem_open failed");
        return 1;
    }

    int gen_x, gen_y;
    while (1) {
        sem_wait(sem);

        for (int i=0; i<99; i++){
            bb->target_xs[i] = -1;
            bb->target_ys[i] = -1;
        }

        for (int i=0; i<bb->n_targets; i++){
            gen_x = rand() % (WIN_SIZE_X-1);
            gen_y = rand() % (WIN_SIZE_Y-1);
            while (gen_x == bb->drone_x && gen_y == bb->drone_y){
                gen_x = rand() % (WIN_SIZE_X-1);
                gen_y = rand() % (WIN_SIZE_Y-1);
            }
            bb->target_xs[i] = gen_x;
            bb->target_ys[i] = gen_y;
        }

        sem_post(sem);
        sleep(5);
    }

    sem_close(sem);
    shmdt(bb);

    return 0;
}
