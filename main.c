#include <ncurses.h>

int main(void)
{
    initscr();                  /* start curses-mode        */

    int h = 7, w = 30;
    WINDOW *win = newwin(h, w, (LINES - h) / 2, (COLS - w) / 2);
    wborder(win, '|', '|', '-', '-', '+', '+', '+', '+');                      /* ramme med standard-tegn */
    mvwprintw(win, 1, 2, "Hej");          /* koordinater er RELATIVE til vinduet */
    wrefresh(win);                        /* opdater kun dette vindue */
    getch();                    /* vent på en tast          */
    endwin();                   /* afslut curses-mode       */
    delwin(win);  /* frigør hukommelsen */
    return 0;
}


