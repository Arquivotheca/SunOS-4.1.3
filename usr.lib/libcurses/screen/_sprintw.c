#ifndef lint
static	char sccsid[] = "@(#)_sprintw.c 1.1 92/07/30 SMI"; /* from S5R2 1.1 */
#endif

# include	"curses.ext"
# include	<varargs.h>

/*
 *	This routine actually executes the printf and adds it to the window
 *
 *	This code now uses the vsprintf routine, which portably digs
 *	into stdio.  We provide a vsprintf for older systems that don't
 *	have one.
 */

_sprintw(win, fmt, ap)
WINDOW	*win;
char	*fmt;
va_list ap;
{
	char	buf[BUFSIZ];

	vsprintf(buf, fmt, ap);
	return waddstr(win, buf);
}
