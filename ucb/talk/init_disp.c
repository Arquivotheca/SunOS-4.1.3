#ifndef lint
static	char sccsid[] = "@(#)init_disp.c 1.1 92/07/30 SMI"; /* from UCB 1.2 83/06/23 */
#endif

/*
 * init_disp contains the initialization code for the display package,
 * as well as the signal handling routines
 */

#include "talk.h"
#include <signal.h>

/* 
 * set up curses, catch the appropriate signals, and build the
 * various windows
 */

init_display()
{
    void sig_sent();

    initscr();
    curses_initialized = 1;

    clear();
    refresh();

    noecho();
    crmode();

    signal(SIGINT, sig_sent);
    signal(SIGPIPE, sig_sent);

	/* curses takes care of ^Z */

    my_win.x_nlines = LINES / 2;
    my_win.x_ncols = COLS;
    my_win.x_win = newwin(my_win.x_nlines, my_win.x_ncols, 0, 0);
    scrollok(my_win.x_win, FALSE);
    wclear(my_win.x_win);

    his_win.x_nlines = LINES / 2 - 1;
    his_win.x_ncols = COLS;
    his_win.x_win = newwin(his_win.x_nlines, his_win.x_ncols,
					     my_win.x_nlines+1, 0);
    scrollok(his_win.x_win, FALSE);
    wclear(his_win.x_win);

    line_win = newwin(1, COLS, my_win.x_nlines, 0);
    box(line_win, '-', '-');
    wrefresh(line_win);

	/* let them know we are working on it */

    current_state = "No connection yet";
}

    /* trade edit characters with the other talk. By agreement
     * the first three characters each talk transmits after
     * connection are the three edit characters
     */

set_edit_chars()
{
    char buf[3];
    int cc;
    struct sgttyb tty;
    struct ltchars ltc;
    
    gtty(0, &tty);

    ioctl(0, TIOCGLTC, (struct sgttyb *) &ltc);
	
    my_win.cerase = tty.sg_erase;
    my_win.kill = tty.sg_kill;

    if (ltc.t_werasc == (char) -1) {
	my_win.werase = '\027';	 /* control W */
    } else {
	my_win.werase = ltc.t_werasc;
    }

    buf[0] = my_win.cerase;
    buf[1] = my_win.kill;
    buf[2] = my_win.werase;

    cc = write(sockt, buf, sizeof(buf));

    if (cc != sizeof(buf) ) {
	p_error("Lost the connection");
    }

    cc = read(sockt, buf, sizeof(buf));

    if (cc != sizeof(buf) ) {
	p_error("Lost the connection");
    }

    his_win.cerase = buf[0];
    his_win.kill = buf[1];
    his_win.werase = buf[2];
}

void sig_sent()
{
    message("Connection closing. Exiting");
    quit();
}

/*
 * All done talking...hang up the phone and reset terminal thingy's
 */

quit()
{
	if (curses_initialized) {
	    wmove(his_win.x_win, his_win.x_nlines-1, 0);
	    wclrtoeol(his_win.x_win);
	    wrefresh(his_win.x_win);
	    endwin();
	}

	if (invitation_waiting) {
	    send_delete();
	}

	exit(0);
}
