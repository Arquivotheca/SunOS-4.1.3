#ifndef lint
static	char	sccsid[] = "@(#)menu_trap.c 1.1 92/07/30";
#endif

/*
 *	Name:		menu_trap.c
 *
 *	Description:	Trap for signals.  Sends the trapped signal to the
 *		process group, cleans up the menu system and exits.
 *
 *	Call syntax:	menu_sig_trap(sig);
 *
 *	Parameters:	int		sig;
 */

#include <signal.h>
#include "menu.h"


/*
 *	External references:
 */
extern	char *		sys_siglist[];


void
menu_sig_trap(sig)
	int		sig;
{
	(void) signal(sig, SIG_IGN);

	if (sig != SIGINT)
		menu_log("%s: trapped signal.", sys_siglist[sig]);

	(void) killpg(getgid(), sig);
	menu_abort(1);
} /* end menu_sig_trap() */
