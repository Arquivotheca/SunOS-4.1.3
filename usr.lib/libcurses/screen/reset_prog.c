#ifndef lint
static	char sccsid[] = "@(#)reset_prog.c 1.1 92/07/30 SMI"; /* from S5R2 1.1 */
#endif

#include "curses.ext"
#include "../local/uparm.h"

extern	struct term *cur_term;

#ifdef DIOCSETT
/*
 * Disable CB/UNIX virtual terminals.
 * This really should be a field in the cur_term structure, but
 * I didn't do it that way because I'm trying to make ex fit.
 * If you're using virtual terminals on a 32 bit machine it
 * probably makes sense to change it.
 */
static struct termcb new, old;
#endif
#ifdef LTILDE
static int newlmode, oldlmode;
#endif

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

reset_prog_mode()
{
	if (BR(Nttyb))
		ioctl(cur_term -> Filedes,
#ifdef USG
			TCSETAW,
#else
			TIOCSETN,
#endif
			&(cur_term->Nttyb));
# ifdef LTILDE
	ioctl(cur_term -> Filedes, TIOCLGET, &oldlmode);
	newlmode = oldlmode & ~LTILDE;
	if (newlmode != oldlmode)
		ioctl(cur_term -> Filedes, TIOCLSET, &newlmode);
# endif
#ifdef DIOCSETT
	if (old.st_termt == 0)
		ioctl(2, DIOCGETT, &old);
	new = old;
	new.st_termt = 0;
	new.st_flgs |= TM_SET;
	ioctl(2, DIOCSETT, &new);
#endif
}
