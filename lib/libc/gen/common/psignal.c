#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)psignal.c 1.1 92/07/30 (C) 1983 SMI";
#endif

/*
 * Print the name of the signal indicated
 * along with the supplied message.
 */
#include <stdio.h>

extern	int fflush();
extern	void _psignal();

void
psignal(sig, s)
	unsigned sig;
	char *s;
{

	(void)fflush(stderr);
	_psignal(sig, s);
}
