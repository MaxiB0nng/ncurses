#include <ncurses.h>
#include <string.h>

void make_win()
{
    int h = 14, w = 50;
    WINDOW *win = newwin(h, w, (LINES - h) / 2, (COLS - w) / 2);
    box(win, 0, 0);
    mvwprintw(win, (h/2), (w/2), "Hello, window!");
    wrefresh(win);
    getch();                    /* vent på en tast          */
    delwin(win);
}



int main(void)
{
    initscr();                  /* start curses-mode        */
    refresh();
    make_win();

    endwin();                   /* afslut curses-mode       */
    return 0;
}
