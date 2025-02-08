#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/mman.h>
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
    int fd = open_watchdog_pipe(PIPE_TARGET);
    logger("Target process started. PID: %d", getpid());

    srand((unsigned int)time(NULL));
    int gen_x, gen_y;
    while (1) {
        sem_wait(sem);
        for (int i=0; i<99; i++){
            bb->target_xs[i] = -1;
            bb->target_ys[i] = -1;
        }
        for (int i=0; i<bb->n_targets; i++){
            gen_x = rand() % (bb->max_width-1);
            gen_y = rand() % (bb->max_height-1);
            while (gen_x == bb->drone_x && gen_y == bb->drone_y){
                gen_x = rand() % (bb->max_width-1);
                gen_y = rand() % (bb->max_height-1);
            }
            bb->target_xs[i] = gen_x;
            bb->target_ys[i] = gen_y;
        }
        sem_post(sem);
        send_heartbeat(fd);
        sleep(TARGET_GENERATION_DELAY);
    }

    if (fd >= 0) { close(fd); }
    sem_close(sem);
    munmap(bb, sizeof(newBlackboard));

    return 0;
}
