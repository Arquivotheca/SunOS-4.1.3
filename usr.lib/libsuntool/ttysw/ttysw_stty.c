#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)ttysw_stty.c 1.1 92/07/30 Copyr 1983 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * Ttysw parameter setting mechanism using given tty settings.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <stdio.h>

extern	char *  sprintf();

/*
 * Determine ttyfd tty settings and cache in environment.
 */
ttysw_saveparms(ttyfd)
	int ttyfd;
{
	int ldisc, localmodes;
	struct sgttyb mode;
	struct tchars tchars;
	struct ltchars ltchars;

	/*
	 * Get line discipline.
	 */
	(void)ioctl(ttyfd, TIOCGETD, &ldisc);
	/*
	 * Get tty parameters
	 */
	(void)ioctl(ttyfd, TIOCGETP, &mode);
	/*
	 * Get local modes
	 */
	(void)ioctl(ttyfd, TIOCLGET, &localmodes);
	/*
	 * Get terminal characters
	 */
	(void)ioctl(ttyfd, TIOCGETC, &tchars);
	/*
	 * Get local special characters
	 */
	(void)ioctl(ttyfd, TIOCGLTC, &ltchars);
	/*
	 * Write environment variable
	 */
	(void)we_setptyparms(ldisc, localmodes, &mode, &tchars, &ltchars);
}

#define	WE_TTYPARMS	"WINDOW_TTYPARMS"
#define	WE_TTYPARMSLEN	120

/*
 * Save tty settings in environment.
 */
we_setptyparms(ldisc, localmodes, mode, tchars, ltchars)
	int ldisc, localmodes;
	struct sgttyb *mode;
	struct tchars *tchars;
	struct ltchars *ltchars;
{
	char str[WE_TTYPARMSLEN];

	str[0] = '\0';
	/*
	 * %c cannot be used to write the character valued fields because
	 *   they often have a value of \0.
	 */
	(void)sprintf(str, "%ld,%ld,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
	    ldisc, localmodes,
	    mode->sg_ispeed, mode->sg_ospeed, mode->sg_erase, mode->sg_kill,
	    mode->sg_flags,
	    tchars->t_intrc, tchars->t_quitc, tchars->t_startc,
	    tchars->t_stopc, tchars->t_eofc, tchars->t_brkc,
	    ltchars->t_suspc, ltchars->t_dsuspc, ltchars->t_rprntc,
	    ltchars->t_flushc, ltchars->t_werasc, ltchars->t_lnextc);
	(void)setenv(WE_TTYPARMS, str);
}
