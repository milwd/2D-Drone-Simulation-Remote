#include <ncurses.h>
#include <unistd.h>
#include <stdlib.h>



/*
need 2 windows, one for loading screen, one for game screen
need to render game screen with drone position, targets, obstacles, time, score, and map
*/
void render_loading(WINDOW *win);
void render_game(WINDOW * win);

int ox[] = {10, 20, 30, 40, 50};
int oy[] = {10, 20, 30, 40, 50};

int main(){
    initscr();                          // loading screen
    WINDOW *win = newwin(0, 0, 0, 0);   // game screen 
    cbreak();  
    noecho();   
    curs_set(0);
    
    nodelay(stdscr, true);
    while (getch() != 'c'){
        render_loading(stdscr);
    }
    touchwin(win);  // check if this is necessary
    attrset(A_NORMAL);
    while (getch() != 'c'){
        render_game(win);
    }
    endwin();
    // if (has_colors())
    // {
    //     flash();
    //     beep();
    //     start_color();
    //     init_pair(1, COLOR_RED, COLOR_BLACK);
    //     attron(COLOR_PAIR(1));
    // }
    // attron(A_BOLD | A_BLINK );

}

void render_loading(WINDOW *win){
    erase();
    attron(A_BOLD | A_BLINK);
    mvprintw(LINES/2, COLS/2, "DRONE SIMULATOR 101");
    mvprintw(LINES/2 + 1, COLS/2, "width = %d, height = %d, pid = %d", COLS, LINES, getpid());

    wrefresh(win);
}

void render_game(WINDOW * win){
    wclear(win);
    wborder(win, 0, 0, 0, 0, 0, 0, 0, 0);
    for (int i = 0; i < 5; i++){
        mvaddch(oy[i], ox[i], 'X'|A_BOLD);
    }
    wrefresh(win);
    // usleep(100000);
}