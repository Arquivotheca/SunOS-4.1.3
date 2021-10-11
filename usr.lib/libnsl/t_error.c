#ifndef lint
static	char sccsid[] = "@(#)t_error.c 1.1 92/07/30 SMI";
#endif

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
#ident	"@(#)libnsl:nsl/t_error.c	1.2"
*/
#include <sys/ioctl.h>
#include <sys/stropts.h>
#include <nettli/tiuser.h>
#include <sys/errno.h>

extern int t_errno, strlen(), write();
extern  char *t_errlist[];
extern t_nerr;
extern void perror();

void
t_error(s)
char	*s;
{
	register char *c;
	register int n;

	c = "Unknown error";
	if(t_errno <= t_nerr)
		c = t_errlist[t_errno];
	n = strlen(s);
	if(n) {
		(void) write(2, s, (unsigned)n);
		(void) write(2, ": ", 2);
	}
	(void) write(2, c, (unsigned)strlen(c));
	if (t_errno == TSYSERR) {
		(void) write(2, ": ", 2);
		perror("");
	} else
		(void) write(2, "\n", 1);
}
