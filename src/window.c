#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <ncurses.h>
#include <unistd.h>
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

    initscr();
    cbreak();
    noecho();
    curs_set(FALSE);

    WINDOW *win = newwin(WIN_SIZE_Y+20, WIN_SIZE_X+10, 1, 1);

    while (1) {
        sem_wait(sem);

        werase(win);

        box(win, 0, 0);
        mvwprintw(win, 1, 2, "Drone Simulator - Inspection Window"); 
        mvwprintw(win, 5, 2, "Drone Position: (%d, %d)", bb->drone_x, bb->drone_y);
        mvwprintw(win, 6, 2, "Score: %d", bb->score);
        mvwprintw(win, 8, 2, " Map:");
        mvwprintw(win, 2, 2, "# targets and obstacles: %d - %d", bb->n_targets, bb->n_obstacles);
        
        for (int i=0; i<bb->n_obstacles; i++){
            mvwprintw(win, 10 + bb->obstacle_ys[i], 4 + bb->obstacle_xs[i], "O(%d %d)", bb->obstacle_ys[i], bb->obstacle_xs[i]);
        }  
        for (int i=0; i<bb->n_targets; i++){
            mvwprintw(win, 10 + bb->target_ys[i], 4 + bb->target_xs[i], "T(%d %d)", bb->target_ys[i], bb->target_xs[i]);
        }  
        // mvwprintw(win, 2, 2, " (%d %d) (%d %d) (%d %d) (%d %d) (%d %d) ", bb->obstacle_xs[0], bb->obstacle_ys[0], bb->obstacle_xs[1], bb->obstacle_ys[1], bb->obstacle_xs[2], bb->obstacle_ys[2], bb->obstacle_xs[3], bb->obstacle_ys[3], bb->obstacle_xs[4], bb->obstacle_ys[4]);
        // mvwprintw(win, 3, 2, " (%d %d) (%d %d) (%d %d) (%d %d) (%d %d) ", bb->target_xs[0], bb->target_ys[0], bb->target_xs[1], bb->target_ys[1], bb->target_xs[2], bb->target_ys[2], bb->target_xs[3], bb->target_ys[3], bb->target_xs[4], bb->target_ys[4]);
        
        mvwprintw(win, 9 + bb->drone_y, 3 + bb->drone_x, "D");

        wrefresh(win);

        sem_post(sem);
        usleep(2000000); // Update every 100ms
    }

    delwin(win);
    endwin();
    sem_close(sem);
    shmdt(bb);

    return 0;
}
