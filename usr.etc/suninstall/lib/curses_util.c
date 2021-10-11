/*      @(#)curses_util.c 1.1 92/07/30 SMI                              */

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include <curses.h>
#include "install.h"

int
xlate(c)
register c;
{
	auto struct sgttyb local;
	auto struct tchars tlocal;
	extern int _tty_ch;	/* terminal file desc.	*/

	(void) ioctl(_tty_ch,TIOCGETP,&local);
	(void) ioctl(_tty_ch,TIOCGETC,&tlocal);

	if (c == local.sg_erase || c == CTRL(h))
		return CERASE;
	else if (c == local.sg_kill)
		return CKILL;
	else if (c == tlocal.t_eofc)
		return CEOT;
	else
		return c;
}

retcon()
{
	char c;

	move(LINES-1,0);
	clrtoeol();
	standout();
	addstr("Press <RETURN> to continue");
	standend();
	refresh();
	c = getch();
#ifdef lint
	c = c;
#endif lint
}

display(y,x,str,field)
        int y, x;
        char *str, *field;
{
        /*
         * Save coordinates
         */
	save(y,x,str,field);
        /*
         * Display str at coordinates
         */
        mvaddstr(y,x,str);
}
