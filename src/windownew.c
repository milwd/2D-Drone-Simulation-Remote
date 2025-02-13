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
#include <time.h>
#include "blackboard.h"


void render_loading(WINDOW *win);
void render_game(WINDOW *win, newBlackboard *bb);
void render_visualization(WINDOW * win, newBlackboard * bb);

int main(int argc, char *argv[]) {
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
    int fd = open_watchdog_pipe(PIPE_WINDOW);
    logger("Window process started. PID: %d", getpid());

    // close(STDIN_FILENO); // close stdin to avoid keyboard input

    initscr();                              // loading screen
    WINDOW * win = newwin(0, 0, 0, 0);      // game screen
    WINDOW * win2 = newwin(0, 0, 0, 0);     // visualization screen
    wborder(win2, 0, 0, 0, 0, 0, 0, 0, 0);

    if (has_colors()){
        start_color(); init_pair(1, COLOR_BLUE, COLOR_BLACK); init_pair(2, COLOR_GREEN, COLOR_BLACK); init_pair(3, COLOR_RED, COLOR_BLACK); init_pair(4, COLOR_BLUE, COLOR_BLACK);
    }
    cbreak();
    noecho();
    curs_set(0);
    wrefresh(stdscr);
    // nodelay(win, TRUE);
    
    time_t now = time(NULL);
    while (1){  // TODO FIND OUT WHY VISUALIZATION CLEARS WHEN RENDER-GAME
        sem_wait(sem);
        getmaxyx(stdscr, bb->max_height, bb->max_width);
        if (bb->state == 0){
            render_loading(stdscr);
            werase(win2);
        } else {
            bb->stats.time_elapsed += RENDER_DELAY/1000000.0;
        }
        if (bb->state == 1){
            render_game(win, bb);
            mvwaddch(win2, bb->drone_y, bb->drone_x, '.'|COLOR_PAIR(4));
        }
        if (bb->state == 2){
            // char text [30];
            // logger(sprintf(text, "Final score %.2f\n",  bb->score));
            break;
        }
        if (bb->state == 3){
            werase(win); wrefresh(win2);
            render_visualization(win2, bb);
        }
        sem_post(sem);
        if (difftime(time(NULL), now) >= 1){
            send_heartbeat(fd);
            now = time(NULL);
        }
        usleep(RENDER_DELAY);
    }

    if (fd >= 0) { close(fd); }
    delwin(win);
    delwin(win2);
    endwin();
    sem_close(sem);
    munmap(bb, sizeof(newBlackboard));

    return 0;
}

void render_loading(WINDOW *win){
    werase(win);
    attron(A_BOLD | A_BLINK);
    char * text = "DRONE SIMULATOR 101";
    mvprintw(LINES/2, COLS/2-sizeof(text)/2, "%s", text);
    wrefresh(win);
}

void render_game(WINDOW * win, newBlackboard *bb){
    werase(win);
    wborder(win, 0, 0, 0, 0, 0, 0, 0, 0);
    for (int i = 0; i < bb->n_obstacles; i++){
        if (bb->obstacle_xs[i] < 1 || bb->obstacle_ys[i] < 1){
            continue;                                                                       // wont break bc if drone hits one, that index becomes -1
        }
        if (bb->obstacle_xs[i] >= bb->max_width || bb->obstacle_ys[i] >= bb->max_height){     // published obstacles are out of bounds
            continue;
        }
        mvwaddch(win, bb->obstacle_ys[i], bb->obstacle_xs[i], 'O'|COLOR_PAIR(3));
    }
    for (int i = 0; i < bb->n_targets; i++){
        if (bb->target_xs[i] < 1 || bb->target_ys[i] < 1){
            continue;                                                                       // wont break bc if drone hits one, that index becomes -1
        }
        if (bb->target_xs[i] >= bb->max_width || bb->target_ys[i] >= bb->max_height){         // published targets are out of bounds
            continue;
        }
        mvwaddch(win, bb->target_ys[i], bb->target_xs[i], 'T'|COLOR_PAIR(2));
    }
    mvwaddch(win, bb->drone_y, bb->drone_x, 'D'|A_BOLD|COLOR_PAIR(1));
    wrefresh(win);
}

void render_visualization(WINDOW * win, newBlackboard * bb){
    mvwaddch(win, bb->drone_y, bb->drone_x, ','|COLOR_PAIR(4));
    wrefresh(win);
}
