#ifndef lint
static  char sccsid[] = "@(#)clock.c 1.1 92/07/30 SMI";
#endif

#include "old.h"
#include <sys/signal.h>

/* 
 * contains miscellaneous routines from qsort.s
 */

onhup()
{
	signal(SIGHUP, SIG_IGN);
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	term();
}

/* 
 * couldn't figure out what this was doing
 */
onint()
{
	intrp++;
}

itinit()
{
	if (signal(SIGHUP, SIG_IGN) != SIG_IGN)
		signal(SIGHUP, onhup);
	if (signal(SIGINT, SIG_IGN) != SIG_IGN)
		signal(SIGINT, onint);
	signal(SIGQUIT, SIG_IGN);
}

clock()
{
	static int t;
	int cur, elapse;

	cur = time(0);
	elapse = cur - t;
	t = cur;
	return elapse;
}
