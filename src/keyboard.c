#include <stdio.h>
#include <stdlib.h>
#include <ncurses.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include "blackboard.h"

void update_forces(int key, int *Fx, int *Fy);
void reset_game(newBlackboard *bb);
void draw_button(WINDOW *parent, int y, int x, const char *label);

int main() {
    // Access shared memory
    // int shmid = shmget(SHM_KEY, sizeof(newBlackboard), 0666);
    // if (shmid == -1) {
    //     perror("shmget failed");
    //     return 1;
    // }

    // newBlackboard *bb = (newBlackboard *)shmat(shmid, NULL, 0);
    // if (bb == (newBlackboard *)-1) {
    //     perror("shmat failed");
    //     return 1;
    // }
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

    int Fx = 0, Fy = 0;

    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0); 

    WINDOW *win = newwin(25, 60, 1, 1);  
    box(win, 0, 0);                      

    refresh();

    while (1) {
        sem_wait(sem);
        wclear(win);
        box(win, 0, 0);
        mvwprintw(win, 1, 3, "DRONE SIMULATION COMMAND CENTER");
        
        // mvwprintw(win, 4, 3, "Score: %.1f", bb->score);
        mvwprintw(win, 5, 3, "Targets: %d Obstacles: %d", bb->stats.hit_targets, bb->stats.hit_obstacles);
        // mvwprintw(win, 6, 3, "Time Elapsed: %.1f", bb->stats.time_elapsed);
        mvwprintw(win, 9, 3, "Command Forces: Fx = %d, Fy = %d", Fx, Fy);
        // mvwprintw(win, 10, 3, "all Forces: Fx = %d, Fy = %d", Fx, Fy);

        draw_button(win, 5, 42, "Start (I)");
        draw_button(win, 9, 42, "Reset (M)");
        draw_button(win, 13, 4, "Up-Left (Q)");
        draw_button(win, 13, 23, "Up (W)");
        draw_button(win, 13, 42, "Up-Right (E)");
        draw_button(win, 17, 4, "Left (A)");
        draw_button(win, 17, 23, "Brake (X)");
        draw_button(win, 17, 42, "Right (D)");
        draw_button(win, 21, 4, "Down-Left (Z)");
        draw_button(win, 21, 23, "Down (S)");
        draw_button(win, 21, 42, "Down-Right (C)");
        
        wrefresh(win);

        int ch = getch();
        if (ch == 'm') {
            reset_game(bb);
            mvprintw(24, 1, "Game reset.");
            refresh();
        }

        // mvwprintw(win, 30, 3, "Key pressed: %c\t", ch);
        update_forces(ch, &Fx, &Fy);
        bb->command_force_x = Fx;
        bb->command_force_y = Fy;
        
        sem_post(sem);

        refresh();
    }

    delwin(win);
    endwin();
    sem_close(sem);
    munmap(bb, sizeof(newBlackboard));
    // shmdt(bb);

    return 0;
}

void draw_button(WINDOW *parent, int y, int x, const char *label) {
    int width = 16;  
    int height = 4;  
    WINDOW *button_win = subwin(parent, height, width, y, x);
    box(button_win, 0, 0);  
    int label_x = (width - strlen(label)) / 2;
    int label_y = height / 2;
    mvwprintw(button_win, label_y, label_x, "%s", label);
    wrefresh(button_win);
}

void update_forces(int key, int *Fx, int *Fy) {
    switch (key) {
        case 'w': case KEY_UP: *Fy -= 1; break; // Up
        case 's': case KEY_DOWN: *Fy += 1; break; // Down
        case 'a': case KEY_LEFT: *Fx -= 1; break; // Left
        case 'd': case KEY_RIGHT: *Fx += 1; break; // Right
        case 'q': *Fx -= 1; *Fy -= 1; break; // Up-Left
        case 'e': *Fx += 1; *Fy -= 1; break; // Up-Right
        case 'z': *Fx -= 1; *Fy += 1; break; // Down-Left
        case 'c': *Fx += 1; *Fy += 1; break; // Down-Right
        case 'x': *Fx = 0; *Fy = 0; break; // Brake
    }
}

void reset_game(newBlackboard *bb) {
    bb->score = 0;
    bb->drone_x = 1;
    bb->drone_y = 1;
    bb->command_force_x = 0;
    bb->command_force_y = 0;
    bb->stats.time_elapsed = 0;
    bb->stats.hit_obstacles = 0;
    bb->stats.hit_targets = 0;
    bb->stats.distance_traveled = 0;

    for (int i = 0; i < MAX_OBSTACLES; i++) {
        bb->obstacle_xs[i] = -1;
        bb->obstacle_ys[i] = -1;
    }

    for (int i = 0; i < MAX_TARGETS; i++) {
        bb->target_xs[i] = -1;
        bb->target_ys[i] = -1;
    }
}
