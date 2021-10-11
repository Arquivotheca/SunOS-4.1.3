#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)ttysw_gtty.c 1.1 92/07/30 Copyr 1983 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * Ttysw parameter retrieval mechanism to get original tty settings to pty.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <signal.h>

static struct sgttyb default_mode = {
		        13, 13, '\0177', CTRL(U),
		        EVENP | ODDP | CRMOD | ECHO
		    };
static struct tchars default_tchars = {
		        CTRL(C), CTRL(\\), CTRL(S),
		        CTRL(Q), CTRL(D), '\E'
		    };
static struct ltchars default_ltchars = {
		        CTRL(Z), CTRL(Y), CTRL(R),
		        CTRL(O), CTRL(W), CTRL(V)
		    };

/*
 * Retrieve tty settings from environment and set ttyfd to them.
 */
ttysw_restoreparms(ttyfd)
	int ttyfd;
{
	int ldisc, localmodes, retrying = 0;
	int fd = 2;
	struct sgttyb mode;
	struct tchars tchars;
	struct ltchars ltchars;
	struct sigvec	vec,
			old_vec;

	/*
	 * Read environment variable
	 */
	while (we_getptyparms(
	    &ldisc, &localmodes, &mode, &tchars, &ltchars) == -1) {
		if (retrying++)
			return (1);
		/*
		 * Try to get the tty parameters from stderr (2).
		 * Using stdin (0) fails when being started in the background
		 *   because csh redirects stdin from the tty to /dev/null.
		 */
		if (!isatty(fd)) {
		    fd = open("/dev/console", 2);
		}
		if (fd > 0) {
		    (void)ttysw_saveparms(fd);
		} else {
		    ldisc = NTTYDISC;
		    localmodes =
		        LPENDIN | LCRTBS | LCRTERA |
		        LCRTKIL | LCTLECH | LDECCTQ;
		    mode = default_mode;
		    tchars = default_tchars;
		    ltchars = default_ltchars;
		    we_setptyparms(ldisc, localmodes,
		    		   &mode, &tchars, &ltchars);
		}
		if (fd != 2) {
		    (void)close(fd);
		}
	}

	/*
         * Force SIGTTOU signals to be ignored before making the following
         * ioctl calls.  Save the old vector and restore it after the ioctls
         * just in case it was important.
         * For more info about SIGTTOU et cetera, see termio(4).
         */
        vec.sv_handler  = SIG_IGN;
        vec.sv_mask     = vec.sv_onstack = 0;
        sigvec(SIGTTOU, & vec, & old_vec);

	/*
	 * Set line discipline.
	 */
	(void)ioctl(ttyfd, TIOCSETD, &ldisc);
	/*
	 * Set tty parameters
	 */
	(void)ioctl(ttyfd, TIOCSETP, &mode);
	/*
	 * Set local modes
	 */
	(void)ioctl(ttyfd, TIOCLSET, &localmodes);
	/*
	 * Set terminal characters
	 */
	(void)ioctl(ttyfd, TIOCSETC, &tchars);
	/*
	 * Set local special characters
	 */
	(void)ioctl(ttyfd, TIOCSLTC, &ltchars);

        /*
         * Now it's safe to restore the old sig vector.
         */
        sigvec(SIGTTOU, & old_vec, 0);

	return (0);
}

#define	WE_TTYPARMS	"WINDOW_TTYPARMS"
#define	WE_TTYPARMSLEN	120

/*
 * Get tty settings from environment.
 */
int
we_getptyparms(ldisc, localmodes, mode, tchars, ltchars)
	int *ldisc, *localmodes;
	struct sgttyb *mode;
	struct tchars *tchars;
	struct ltchars *ltchars;
{
	char str[WE_TTYPARMSLEN];
	short temps[16];	/* Needed for sscanf as there is no %hhd */

	if (_we_setstrfromenvironment(WE_TTYPARMS, str))
		return (-1);
	else {
		if (sscanf(str,
  "%ld,%ld,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd",
		    ldisc, localmodes, &temps[0], &temps[1], &temps[2],
		    &temps[3], &mode->sg_flags, &temps[4], &temps[5], &temps[6],
		    &temps[7], &temps[8], &temps[9], &temps[10], &temps[11],
		    &temps[12], &temps[13], &temps[14], &temps[15])
		     != 19)
			return (-1);
		    mode->sg_ispeed = temps[0];
		    mode->sg_ospeed = temps[1];
		    mode->sg_erase = temps[2];
		    mode->sg_kill = temps[3];
		    tchars->t_intrc = temps[4];
		    tchars->t_quitc = temps[5];
		    tchars->t_startc = temps[6];
		    tchars->t_stopc = temps[7];
		    tchars->t_eofc = temps[8];
		    tchars->t_brkc = temps[9];
		    ltchars->t_suspc = temps[10];
		    ltchars->t_dsuspc = temps[11];
		    ltchars->t_rprntc = temps[12];
		    ltchars->t_flushc = temps[13];
		    ltchars->t_werasc = temps[14];
		    ltchars->t_lnextc = temps[15];
		/*
		 * Always clear
		 */
		(void)unsetenv(WE_TTYPARMS);
		return (0);
	}
}
