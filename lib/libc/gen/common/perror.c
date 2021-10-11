#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)perror.c 1.1 92/07/30 SMI";
#endif

/*
 * Print the error indicated
 * in the cerror cell.
 */
#include <stdio.h>

extern	int fflush();
extern	void _perror();

void
perror(s)
	char *s;
{

	(void)fflush(stderr);
	_perror(s);
}
