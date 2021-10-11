#ifndef lint
static char sccsid[] = "@(#)help.c 1.1 92/07/30 SMI";
#endif lint

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include "curses.h"
#include "install.h"

static char ONLINEHELP1[]  = " ON-LINE HELP ";
static char ONLINEHELP2[]  = "-----------------------------------------------------------------------------";
static char FINISH[]   = "Are you finished with this form [y/n] ? ";

jmp_buf help_sjbuf;

help()
{
	void help_repaint(), help_intr();
	char c;

	(void) signal(SIGINT,help_intr);
	(void) signal(SIGHUP,SIG_IGN);
	(void) signal(SIGQUIT,help_repaint);
	help_redisplay();
	(void) setjmp(help_sjbuf);
	do {
        	move(LINES-2,sizeof(FINISH));
        	clrtoeol();
        	refresh();
        	noecho();
        	c = getch();
        	switch (c) { 
        	case 'y':
        		mvaddch(LINES-2,sizeof(FINISH),'y');
        		break;
        	case 'n':
        		mvaddch(LINES-2,sizeof(FINISH),'n');
        		break;
        	}
        	refresh();
        	echo();
        } while ( c != 'y' ) ;
}

help_redisplay()
{
	clear(); 
        mvaddstr(0,1,ONLINEHELP1);
        mvaddstr(1,1,ONLINEHELP2);
        mvaddstr(4,10,"-----------------------------------------------------");
        mvaddstr(5,10,"   KEYS                        PURPOSE               ");
        mvaddstr(6,10,"-----------------------------------------------------");
        mvaddstr(7,10,"   CONTROL F                   move cursor forward");
        mvaddstr(8,10,"   CONTROL N                   move cursor forward");
        mvaddstr(9,10,"   CONTROL B                   move cursor backward");
        mvaddstr(10,10,"   CONTROL P                   move cursor backward");
        mvaddstr(11,10,"   CONTROL U                   erase word");
	mvaddstr(12,10,"   <DELETE>                    erase one character");
        mvaddstr(13,10,"   CONTROL \\                   repaint screen");
	mvaddstr(14,10,"   CONTROL C                   abort");
	mvaddstr(15,10,"   <RETURN>                    end of input string");
	mvaddstr(16,10,"   x or X                      select a choice");
	mvaddstr(17,10,"   <SPACE>                     next choice");
	mvaddstr(LINES-2,1,FINISH);
	refresh();
}

void
help_repaint()
{
	help_redisplay();
	longjmp(help_sjbuf,0);
}

void
help_intr()
{
	aborting(0);
}
