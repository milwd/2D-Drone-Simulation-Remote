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

    while (1) {
        sem_wait(sem);

        for (int i=0; i<99; i++){
            bb->target_xs[i] = -1;
            bb->target_ys[i] = -1;
        }

        for (int i=0; i<bb->n_targets; i++){
            bb->target_xs[i] = rand() % (WIN_SIZE_X-1);
            bb->target_ys[i] = rand() % (WIN_SIZE_Y-1);
        }

        sem_post(sem);
        sleep(5);
    }

    sem_close(sem);
    shmdt(bb);

    return 0;
}
