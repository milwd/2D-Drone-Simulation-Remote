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


void wait_for_game_start(WINDOW *win, int * pipe_fd);
void check_for_reset(int * pipe_fd);
void check_for_quit(int * pipe_fd);
void render_game(WINDOW *win, newBlackboard *bb);

bool game_started = false;

int main(int argc, char *argv[]) {
    logger("Window process started...");
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
    int pipe_fd = open(PIPE_NAME, O_RDONLY);
    if (pipe_fd == -1) {
        perror("open failed");
        return 1;
    }

    initscr();
    cbreak();
    noecho();
    curs_set(FALSE);

    WINDOW *win = newwin(WIN_SIZE_Y + 10, WIN_SIZE_X + 3, 1, 1);
    
    while (1){
        wrefresh(win);
        wait_for_game_start(win, &pipe_fd);

        while (game_started) {
            check_for_reset(&pipe_fd);
            sem_wait(sem);

            render_game(win, bb);
            
            sem_post(sem); 
            usleep(RENDER_DELAY); // 100ms
        }

        check_for_quit(&pipe_fd);
    }

    close(pipe_fd);

    delwin(win);
    endwin();
    sem_close(sem);
    munmap(bb, sizeof(newBlackboard));

    return 0;
}

void wait_for_game_start(WINDOW *win, int * pipe_fd) {
    mvwprintw(win, 1, 1, "Waiting for game to start...");
    char buffer[10];
    while (!game_started) {
        ssize_t n = read(*pipe_fd, buffer, sizeof(buffer));
        if (n > 0 && strncmp(buffer, "start", 5) == 0) {
            game_started = true;
            break;
        }
    }
    close(*pipe_fd); 
}

void check_for_reset(int * pipe_fd) {
    char buffer[10];
    ssize_t n = read(*pipe_fd, buffer, sizeof(buffer));
    if (n > 0 && strncmp(buffer, "reset", 5) == 0) {
        game_started = false;
    }
    close(*pipe_fd); 
}

void check_for_quit(int * pipe_fd) {
    char buffer[10];
    ssize_t n = read(*pipe_fd, buffer, sizeof(buffer));
    if (n > 0 && strncmp(buffer, "quiet", 5) == 0) {
        game_started = false;
    }
    close(*pipe_fd); 
    exit(0);
}

void render_game(WINDOW *win, newBlackboard *bb) {
    werase(win);

    box(win, 0, 0);
    mvwprintw(win, 1, 1, "Drone Simulator - Inspection Window"); 
    mvwprintw(win, 3, 1, "Drone Position: (%d, %d)", bb->drone_x, bb->drone_y);
    mvwprintw(win, 4, 1, "# targets and obstacles: %d - %d", bb->n_targets, bb->n_obstacles);
    mvwprintw(win, 5, 1, "Time: %.1f secs, Targets: %d", bb->stats.time_elapsed, bb->stats.hit_targets);
    mvwprintw(win, 6, 1, "Score: %.2f", bb->score);
    mvwprintw(win, 7, 1, "Map:");
    mvwprintw(win, 8, 0, "-");
    whline(win, '=', WIN_SIZE_X + 2);

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
}
