#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)abort.c 1.1 92/07/30 SMI"; /* from S5R3 1.11 */
#endif

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*LINTLIBRARY*/
/*
 *	abort() - terminate current process with dump via SIGABRT
 */

#include <signal.h>

extern int kill(), getpid();
extern void _cleanup();
static pass = 0;		/* counts how many times abort has been called*/

int
abort()
{
	void (*sig)();

	if ((sig = signal(SIGABRT,SIG_DFL)) != SIG_DFL) 
		(void) signal(SIGABRT,sig); 
	else if (++pass == 1)
		_cleanup();
	return(kill(getpid(), SIGABRT));
}
