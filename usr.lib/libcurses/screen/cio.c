/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)cio.c 1.1 92/07/30 SMI"; /* from S5R3 1.5 */
#endif

/*
 * These routines are the main things
 * in this level of curses that depend on the outside
 * environment, so they are grouped here.
 */
#include "curses.ext"

int outchcount;

/*
 * Write out one character to the tty.
 */
_outch (c)
chtype c;
{
#ifdef DEBUG
# ifndef LONGDEBUG
	if (outf)
		if (c < ' ' || c == 0177)
			fprintf(outf, "^%c", c^0100);
		else
			fprintf(outf, "%c", c&0177);
# else /* LONGDEBUG */
	if(outf) fprintf(outf, "_outch: char '%s' term %x file %x=%d\n",
		unctrl(c&0177), SP, SP->term_file, fileno(SP->term_file));
# endif /* LONGDEBUG */
#endif /* DEBUG */

	outchcount++;
	if (SP && SP->term_file)
		putc ((int)c&0177, SP->term_file);
	else
		putc ((int)c&0177, stdout);
}

/*
 * Flush stdout.
 */
__cflush()
{
	fflush(SP->term_file);
}

static short    baud_convert[] =
{
	0, 50, 75, 110, 135, 150, 200, 300, 600, 1200,
	1800, 2400, 4800, 9600, 19200, 38400
};

/*
 * Force output to be buffered.
 * Also figures out the baud rate.
 * Grouped here because they are machine dependent.
 */

_setbuffered(fd)
FILE *fd;
{
	char *sobuf;
	char *malloc();
	SGTTY   sg;

	sobuf = malloc(BUFSIZ);
	if (sobuf == NULL)
	    (void) fprintf (stderr, "malloc returned NULL in _setbuffered\n");
#ifdef _IOFBF
	{
# include <signal.h>
# ifdef SIGPOLL
	setvbuf(fd, sobuf, _IOFBF, BUFSIZ);
# else
	setvbuf(fd, _IOFBF, sobuf, BUFSIZ);
# endif /* SIGPOLL */
	}
#else
	setbuf(fd, sobuf);
#endif /* _IOFBF */

	/* Ignore this ioctl return code so piping works properly */
# ifdef SYSV
	(void) ioctl (fileno (fd), TCGETA, &sg);
	SP->baud = sg.c_cflag&CBAUD ? baud_convert[sg.c_cflag&CBAUD] : 1200;
# else
	(void) ioctl (fileno (fd), TIOCGETP, &sg);
	SP->baud = sg.sg_ospeed ? baud_convert[sg.sg_ospeed] : 1200;
# endif
	/* 0 baud implies a bug somewhere, so default to 9600. */
	if (SP->baud == 0)
		SP->baud = 9600;
}
