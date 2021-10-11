#ifndef lint
static	char sccsid[] = "@(#)resetshell.c 1.1 92/07/30 SMI"; /* from S5R2 1.1 */
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

reset_shell_mode()
{
#ifdef DIOCSETT
	/*
	 * Restore any virtual terminal setting.  This must be done
	 * before the TIOCSETN because DIOCSETT will clobber flags like xtabs.
	 */
	old.st_flgs |= TM_SET;
	ioctl(2, DIOCSETT, &old);
#endif
	if (BR(Ottyb)) {
		ioctl(cur_term -> Filedes,
#ifdef USG
			TCSETAW,
#else
			TIOCSETN,
#endif
			&(cur_term->Ottyb));
# ifdef LTILDE
	if (newlmode != oldlmode)
		ioctl(cur_term -> Filedes, TIOCLSET, &oldlmode);
# endif
	}
}
