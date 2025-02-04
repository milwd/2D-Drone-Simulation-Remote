#include <ncurses.h>

int main()
{
    char text[] = "Hello, World!";
    char * t = text;
    initscr();
    if (has_colors())
    {
        flash();
        beep();
        start_color();
        init_pair(1, COLOR_RED, COLOR_BLACK);
        attron(COLOR_PAIR(1));
    }
    attron(A_BOLD | A_BLINK );
    addstr(t);
    attrset(A_NORMAL);
    border(0, 0, 0, 0, 0, 0, 0, 0);
    addch('D' | A_UNDERLINE);
    addstr("\nPress any key to exit...");
    getch();
    while (true){
        clear();
        mvprintw(LINES/2, COLS/2, "Window size: %d x %d", COLS, LINES);
        refresh();
        if (getch() == 'c'){
            break;
        }
    }
    endwin();
    return 0;
}