#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <unistd.h>
#include <fcntl.h>
#include "blackboard.h"


int main() {
    // Access shared memory
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

    // Open pipe for reading commands
    int pipe_fd = open(PIPE_NAME, O_RDONLY);
    if (pipe_fd == -1) {
        perror("Pipe open failed");
        return 1;
    }

    char command[10];
    while (1) {
        // Read commands from the keyboard manager
        if (read(pipe_fd, command, sizeof(command)) > 0) {
            sem_wait(sem);
            if (strcmp(command, "UP") == 0) bb->drone_y--;
            if (strcmp(command, "DOWN") == 0) bb->drone_y++;
            if (strcmp(command, "LEFT") == 0) bb->drone_x--;
            if (strcmp(command, "RIGHT") == 0) bb->drone_x++;
            printf("Drone moved to (%d, %d)\n", bb->drone_x, bb->drone_y);
            sem_post(sem);
        }
        usleep(100000);
    }

    close(pipe_fd);
    sem_close(sem);
    shmdt(bb);

    return 0;
}
