/*	@(#)t.c 1.1 92/07/30 SMI	*/
#include	<curses.h>

main() {
	register i;

	initscr();
	mvaddstr(0, 20, "This program will delete line #5 after");
	mvaddstr(1, 20, "writing the line number for each line.");
	for(i = 0; i < LINES; ++i)
		mvprintw(i, 0, "Line #%d", i);
	refresh();
	addstr("     You have the bug if the BOTTOM line is not COMPLETELY blank!");
	move(5, 0);
	deleteln();
	move(LINES - 3, 0);
	refresh();
	endwin();
}
