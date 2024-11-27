#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <ncurses.h>
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

    initscr();
    cbreak();
    noecho();
    curs_set(FALSE);

    WINDOW *win = newwin(WIN_SIZE_Y+10, WIN_SIZE_X+3, 1, 1);

    while (1) {
        sem_wait(sem);

        werase(win);

        box(win, 0, 0);
        mvwprintw(win, 1, 1, "Drone Simulator - Inspection Window"); 
        mvwprintw(win, 3, 1, "Drone Position: (%d, %d)", bb->drone_x, bb->drone_y);
        mvwprintw(win, 4, 1, "# targets and obstacles: %d - %d", bb->n_targets, bb->n_obstacles);
        mvwprintw(win, 5, 1, "Time: %.1f secs, Targets: %d", bb->stats.time_elapsed, bb->stats.hit_targets);
        mvwprintw(win, 6, 1, "Score: %.2f", bb->score);
        mvwprintw(win, 7, 1, "Map:");
        mvwprintw(win, 8, 0, "-");
        whline(win, '=', WIN_SIZE_X+2);
        for (int y = 0; y < WIN_SIZE_Y; y++) {
            for (int x = 1; x <= WIN_SIZE_X; x++) {
                for (int i = 0; i < bb->n_obstacles; i++) {
                    if (bb->obstacle_xs[i] == x && bb->obstacle_ys[i] == y) {
                        mvwprintw(win, 9 + y, 0 + x, "O");
                    }
                }
                for (int i = 0; i < bb->n_targets; i++) {
                    if (bb->target_xs[i] == x && bb->target_ys[i] == y) {
                        mvwprintw(win, 9 + y, 0 + x, "T");
                    }
                }
                if (bb->drone_x == x && bb->drone_y == y) {
                    mvwprintw(win, 9 + y, 0 + x, "D");
                }
            }
        }
        bb->stats.time_elapsed += RENDER_DELAY / 1000000.0;
        wrefresh(win);
        sem_post(sem); 
        usleep(RENDER_DELAY); // 100ms
    }

    delwin(win);
    endwin();
    sem_close(sem);
    shmdt(bb);

    return 0;
}
