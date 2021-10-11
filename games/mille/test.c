#ifndef lint
static	char sccsid[] = "@(#)test.c 1.1 92/07/30 SMI"; /* from UCB */
#endif
# include	<curses.h>

main() {

	register int	i;
	register WINDOW	*hw;

	initscr();
#if 1
	for (i = 0; i < 10; i++)
		mvaddstr(i, i, "hi");
	refresh();
	hw = newwin(LINES, COLS, 0, 0);
	for (i = 0; i < 10; i++)
		mvwaddstr(hw, i, i, "HI");
	wrefresh(hw);
	clearok(curscr, TRUE);
	refresh();
#else
	idlok(stdscr, TRUE);
	leaveok(stdscr, TRUE);
	printw("hi there, beautiful");
	mvaddch(0, 50, '|');
	for (i = 0; i < 10; i++) {
		refresh();
		mvaddch(0, 50, ' ');
		move(0, 0);
		insch('a');
		mvaddch(0, 50, '|');
	}
	for (i = 0; i < 10; i++) {
		refresh();
		mvaddch(0, 50, ' ');
		move(0, 0);
		delch();
		mvaddch(0, 50, '|');
	}
	move(0, 0);
	insertln();
	printw("-------------------\n");
	refresh();
	insertln();
	printw("+++++++++++++++++++\n");
	refresh();
#endif
	endwin();
	exit(0);
	/* NOTREACHED */
}
