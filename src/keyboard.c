#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include "blackboard.h"


void update_forces(int key, int *Fx, int *Fy);

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

    // mkfifo(PIPE_NAME, 0666); // Create named pipe
    // int pipe_fd = open(PIPE_NAME, O_WRONLY); // Open pipe for writing

    // if (pipe_fd == -1) {
    //     perror("Pipe open failed");
    //     return 1;
    // }

    int Fx = 0, Fy = 0;

    initscr();  
    cbreak();   // disable line buffering
    noecho();   // prevent characters from displaying
    keypad(stdscr, TRUE);   // enables capturing special keys

    printw("Use arrow keys to move the drone. Press 'q' to quit.\n");
    refresh();

    int ch;
    while ((ch = getch()) != 'q') {
        update_forces(ch, &Fx, &Fy);
        bb->command_force_x = Fx;
        bb->command_force_y = Fy;
        mvprintw(2, 0, "Command Forces: Fx = %d, Fy = %d\n", Fx, Fy);
        refresh();
    }

    // close(pipe_fd);
    endwin();
    shmdt(bb);

    return 0;
}


void update_forces(int key, int *Fx, int *Fy) {
    switch (key) {
        case 'w': *Fy -= 1; break; // Up
        case 's': *Fy += 1; break; // Down
        case 'a': *Fx -= 1; break; // Left
        case 'd': *Fx += 1; break; // Right
        case 'q': *Fx -= 1; *Fy -= 1; break; // Up-Left
        case 'e': *Fx += 1; *Fy -= 1; break; // Up-Right
        case 'z': *Fx -= 1; *Fy += 1; break; // Down-Left
        case 'c': *Fx += 1; *Fy += 1; break; // Down-Right
        case 'x': *Fx = 0; *Fy = 0; break; // Reset forces
    }
}
