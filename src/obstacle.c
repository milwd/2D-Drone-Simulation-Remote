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
    int fd = open_watchdog_pipe(PIPE_OBSTACLE);
    logger("Obstacle process started. PID: %d", getpid());

    srand((unsigned int)time(NULL));
    int gen_x, gen_y;
    while (1) {
        sem_wait(sem);
        for (int i=0; i<MAX_OBJECTS; i++){
            bb->obstacle_xs[i] = -1;
            bb->obstacle_ys[i] = -1;
        }
        for (int i=0; i<bb->n_obstacles; i++){ 
            gen_x = rand() % (bb->max_width-1);
            gen_y = rand() % (bb->max_height-1);
            while (gen_x == bb->drone_x && gen_y == bb->drone_y){
                gen_x = rand() % (bb->max_width-1);
                gen_y = rand() % (bb->max_height-1);
            }
            bb->obstacle_xs[i] = gen_x;
            bb->obstacle_ys[i] = gen_y;
        }
        sem_post(sem);
        send_heartbeat(fd);
        sleep(OBSTACLE_GENERATION_DELAY);
    }

    if (fd >= 0) { close(fd); }
    sem_close(sem);
    munmap(bb, sizeof(newBlackboard));
    return 0;
}
