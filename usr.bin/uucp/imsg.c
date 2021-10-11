/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident  "@(#)imsg.c 1.1 92/07/30"	/* from SVR3.2 uucp:imsg.c 2.3 */

#include "uucp.h"

#define MSYNC	'\020'
/* maximum likely message - make sure you don't get run away input */
#define MAXIMSG	256

/*
 * read message routine used before a
 * protocol is agreed upon.
 *	msg	-> address of input buffer
 *	fn	-> input file descriptor 
 * returns:
 *	EOF	-> no more messages
 *	0	-> message returned
 */
imsg(msg, fn)
register char *msg;
register int fn;
{
	register char c;
	register int i;
	short fndsync;
	char *bmsg;
	extern int	errno, sys_nerr;
	extern char *sys_errlist[];

	fndsync = 0;
	bmsg = msg;
	CDEBUG(7, "imsg %s>", "");
	while ((i = (*Read)(fn, msg, sizeof(char))) == sizeof(char)) {
		*msg &= 0177;
		c = *msg;
		CDEBUG(7, "%s", c < 040 ? "^" : "");
		CDEBUG(7, "%c", c < 040 ? c | 0100 : c);
		if (c == MSYNC) { /* look for sync character */
			msg = bmsg;
			fndsync = 1;
			continue;
		}
		if (!fndsync)
			continue;

		if (c == '\0' || c == '\n') {
			*msg = '\0';
			return(0);
		}
		else
			msg++;

		if (msg - bmsg > MAXIMSG)	/* unlikely */
			return(FAIL);
	}
	/* have not found sync or end of message */
	if (i < 0) {
		if (errno < sys_nerr)
			CDEBUG(7, "\nimsg read error: %s\n",
			    sys_errlist[errno]);
		else
			CDEBUG(7, "\nimsg read error, errno %d\n", errno);
	}
	*msg = '\0';
	return(EOF);
}

/*
 * initial write message routine -
 * used before a protocol is agreed upon.
 *	type	-> message type
 *	msg	-> message body address
 *	fn	-> file descriptor
 * return: 
 *	Must always return 0 - wmesg (WMESG) looks for zero
 */
omsg(type, msg, fn)
register char *msg;
register char type;
int fn;
{
	char buf[BUFSIZ];

	(void) sprintf(buf, "%c%c%s", MSYNC, type, msg);
	DEBUG( 7, "omsg \"%s\"\n", &buf[1] );
	(*Write)(fn, buf, strlen(buf) + 1);
	return(0);
}

/*
 * null turnoff routine to be used for errors
 * during protocol selection.
 */
turnoff()
{
	return(0);
}
