#ifndef lint
static	char sccsid[] = "@(#)def_shell.c 1.1 92/07/30 SMI"; /* from S5R2 1.1 */
#endif

#include "curses.h"
#include "term.h"

extern	struct term *cur_term;


/*
 * Getting the baud rate is different on the two systems.
 * In either case, a baud rate of 0 hangs up the phone.
 * Since things are often initialized to 0, getting the phone
 * hung up on you is a common result of a bug in your program.
 * This is not very friendly, so if the baud rate is 0, we
 * assume we're doing a reset_xx_mode with no def_xx_mode, and
 * just don't do anything.
 */
#ifdef USG
#define BR(x) (cur_term->x.c_cflag&CBAUD)
#else
#define BR(x) (cur_term->x.sg_ispeed)
#endif

def_shell_mode()
{
#ifdef USG
	ioctl(cur_term -> Filedes, TCGETA, &(cur_term->Ottyb));
#else
	ioctl(cur_term -> Filedes, TIOCGETP, &(cur_term->Ottyb));
#endif
	/* This is a useful default for Nttyb, too */
	if (BR(Nttyb) == 0)
		cur_term -> Nttyb = cur_term -> Ottyb;
}
