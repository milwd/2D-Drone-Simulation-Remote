#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <ncurses.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <string.h>
#include <signal.h>
#include <stdbool.h>
#include "blackboard.h"


void render_loading(WINDOW *win);
void render_game(WINDOW *win, newBlackboard *bb);

bool state = 0;

int main(int argc, char *argv[]) {
    logger("Window process started...");  // TODO ADD PID
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

    // close(STDIN_FILENO); // close stdin to avoid keyboard input

    initscr();                          // loading screen
    WINDOW * win = newwin(0, 0, 0, 0);  // game screen

    if (has_colors()){
        start_color(); init_pair(1, COLOR_BLUE, COLOR_BLACK); init_pair(2, COLOR_GREEN, COLOR_BLACK); init_pair(3, COLOR_RED, COLOR_BLACK);
    }
    cbreak();
    noecho();
    curs_set(0);
    wrefresh(stdscr);
    // nodelay(win, TRUE);
    
    while (1){
        sem_wait(sem);
        bb->max_height  = LINES;
        bb->max_width   = COLS;
        if (bb->state == 0){
            render_loading(stdscr);
        }
        if (bb->state == 1){
            render_game(win, bb);
        }
        if (bb->state == 2){
            break;
        }
        sem_post(sem);
        // TODO ADD QUIT CAPABILITY
        usleep(RENDER_DELAY);
    }

    delwin(win);
    endwin();
    sem_close(sem);
    munmap(bb, sizeof(newBlackboard));

    return 0;
}

void render_loading(WINDOW *win){
    erase();
    attron(A_BOLD | A_BLINK);
    mvprintw(LINES/2, COLS/2, "DRONE SIMULATOR 101");
    wrefresh(win);
}

void render_game(WINDOW * win, newBlackboard *bb){
    werase(win);
    wborder(win, 0, 0, 0, 0, 0, 0, 0, 0);
    for (int i = 0; i < bb->n_obstacles; i++){
        if (bb->obstacle_xs[i] < 1 || bb->obstacle_ys[i] < 1){
            continue;                                                                       // wont break bc if drone hits one, that index becomes -1
        }
        if (bb->obstacle_xs[i] > bb->max_width || bb->obstacle_ys[i] > bb->max_height){     // published obstacles are out of bounds
            continue;
        }
        mvwaddch(win, bb->obstacle_ys[i], bb->obstacle_xs[i], 'O'|COLOR_PAIR(3));
    }
    for (int i = 0; i < bb->n_targets; i++){
        if (bb->target_xs[i] < 1 || bb->target_ys[i] < 1){
            continue; 
        }
        if (bb->target_xs[i] > bb->max_width || bb->target_ys[i] > bb->max_height){         // published targets are out of bounds
            continue;
        }
        mvwaddch(win, bb->target_ys[i], bb->target_xs[i], 'T'|COLOR_PAIR(2));
    }
    mvwaddch(win, bb->drone_y, bb->drone_x, 'D'|A_BOLD|COLOR_PAIR(1));
    wrefresh(win);
}
