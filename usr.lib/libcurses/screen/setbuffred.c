#ifndef lint
static	char sccsid[] = "@(#)setbuffred.c 1.1 92/07/30 SMI"; /* from S5R2 1.1 */
#endif

/*
 * This routine is one of the main things
 * in this level of curses that depends on the outside
 * environment.
 */
#include "curses.ext"

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
	char *calloc();
	SGTTY   sg;

	sobuf = calloc(1, BUFSIZ);
	setbuf(fd, sobuf);

# ifdef USG
	ioctl (fileno (fd), TCGETA, &sg);
	SP->baud = sg.c_cflag&CBAUD ? baud_convert[sg.c_cflag&CBAUD] : 1200;
# else
	ioctl (fileno (fd), TIOCGETP, &sg);
	SP->baud = sg.sg_ospeed ? baud_convert[sg.sg_ospeed] : 1200;
# endif
}
