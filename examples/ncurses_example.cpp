#include <ncurses/ncurses.h>

#include <string>

int main(int argc, char *argv[])
{
    initscr();
    cbreak();
    noecho();
    do {
        printw("hello world!!!\n");
        refresh();
    } while(getch() != 'q');
    endwin();
    return 0;
}
