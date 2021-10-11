#ifndef lint
#ifdef SunB1
static	char		mls_sccsid[] = "@(#)sig_trap.c 1.1 92/07/30 SMI; SunOS MLS";
#else
static	char		sccsid[] = "@(#)sig_trap.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 *	Name:		sig_trap.c
 *
 *	Description:	Trap for signals.  Sends the trapped signal to the
 *		process group, and exits.
 *
 *	Call syntax:	sig_trap(sig);
 *
 *	Parameters:	int		sig;
 */

#include <curses.h>
#include <signal.h>
#include "install.h"
#include "menu.h"


/*
 *	External references:
 */
extern	char *		sys_siglist[];


void
sig_trap(sig)
	int		sig;
{
	(void) signal(sig, SIG_IGN);

	if (sig != SIGINT)
		menu_log("%s: %s: trapped signal.", progname,
			 sys_siglist[sig]);
	clear();
	refresh();
	endwin();
	echo();
	resetty();

	(void) killpg(getgid(), sig);
	menu_abort(1);
} /* end sig_trap() */
