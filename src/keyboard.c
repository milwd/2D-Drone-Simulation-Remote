#include <stdio.h>
#include <ncurses.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "blackboard.h"


int main() {
    mkfifo(PIPE_NAME, 0666); // Create named pipe
    int pipe_fd = open(PIPE_NAME, O_WRONLY); // Open pipe for writing

    if (pipe_fd == -1) {
        perror("Pipe open failed");
        return 1;
    }

    initscr();  
    cbreak();   // disable line buffering
    noecho();   // prevent characters from displaying
    keypad(stdscr, TRUE);   // enables capturing special keys

    printw("Use arrow keys to move the drone. Press 'q' to quit.\n");
    refresh();

    int ch;
    while ((ch = getch()) != 'q') {
        switch (ch) {
            case KEY_UP:
                write(pipe_fd, "UP", 3);
                break;
            case KEY_DOWN:
                write(pipe_fd, "DOWN", 5);
                break;
            case KEY_LEFT:
                write(pipe_fd, "LEFT", 5);
                break;
            case KEY_RIGHT:
                write(pipe_fd, "RIGHT", 6);
                break;
            default:
                break;
        }
    }

    close(pipe_fd);
    endwin();

    return 0;
}
