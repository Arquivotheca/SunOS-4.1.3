#ifndef lint
static  char sccsid[] = "@(#)screensub.c 1.1 92/07/30 Copyr 1987 Sun Micro";
#endif
/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */
/*
 * screen subroutines.....
 *
 */

#include <curses.h>

/*VARARGS1*/
void
infoprint(a,b,c,d,e,f,g,h,i,j,k)
char *a;
{
	move(LINES-2,0);
	clrtobot();
	(void) printw(a,b,c,d,e,f,g,h,i,j,k);
	refresh();
	sleep(2);
	move(LINES-2,0);
	clrtobot();
	refresh();
}

/*VARARGS1*/
void
errprint(a,b,c,d,e,f,g,h,i,j,k)
char *a;
{
	standout();
	infoprint(a,b,c,d,e,f,g,h,i,j,k);
	standend();
}

void
bell()
{
	extern char *VB;
	if(VB)
		addstr(VB);
	else
		addch(7);
	refresh();
}
