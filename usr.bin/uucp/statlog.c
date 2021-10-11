/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident  "@(#)statlog.c 1.1 92/07/30"	/* from SVR3.2 uucp:statlog.c 1.2 */

#include "uucp.h"

int Retries;

/*
	Report and log file transfer rate statistics.
	This is ugly because we are not using floating point.
*/

void
statlog( direction, bytes, millisecs )
char		*direction;
unsigned long	bytes;
time_t		millisecs;
{
	char		text[ 100 ], *tptr;
	unsigned long	bytes1000;

	bytes1000 = bytes * 1000;
	/* on fast machines, times(2) resolution may not be enough */
	/* so millisecs may be zero.  just use 1 as best guess */
	if ( millisecs == 0 )
		millisecs = 1;
	(void) sprintf(text, "%s %lu / %lu.%.3u secs, %lu bytes/sec",
		direction, bytes, millisecs/1000, millisecs%1000,
		bytes1000/millisecs );
	if (Retries) {
		sprintf(text + strlen(text), " %d retries", Retries);
		Retries = 0;
	}
	CDEBUG(4, "%s\n", text);
	syslog(text);
}
