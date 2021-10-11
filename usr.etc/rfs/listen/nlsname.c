/*	@(#)nlsname.c 1.1 92/07/30 SMI 	*/

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)listen:nlsname.c	1.3"

/*
 * nlsname:	given a nodename in a null terminated string,
 *		nlsname performs the listener's permutation (for AT&T/SMB svc)
 *		and returns a pointer to a char string containing
 *		the permuted nodename.  NOTE that the name returned
 *		must be run through lname2addr(3) before passing it to
 *		t_bind(3).
 *
 * WARNING:	The return value of this routine points to static data 
 *		that are overwritten by each call.
 *
 * NOTE:	This code is specific to STARLAN NETWORK.
 *
 */


/* includes	*/

#include <ctype.h>
#include <string.h>
#include <stdio.h>

/* defines	*/

/* NAMEBUFSZ must be greater than sizeof(nodename) + strlen(permute) */
/* the buffer must be large enough to handle an internet address	*/

#define NAMEBUFSZ	256

/*
 * when nlsname is included in the listener,
 * DEBUGMODE can be turned on as a dubugging aid
 */

/* #define DEBUGMODE */

#ifdef	DEBUGMODE
#define	DEBUG(args)	debug args
#else
#define DEBUG(args)
#endif

/*
 * to log errors to stderr, set _nlslog to a non-zero value.
 */

extern	int _nlslog;

static void
logmessage(s)
char *s;
{
	if (_nlslog)
		fprintf(stderr,s);
}


/*
 * nlsname:
 *
 * char *nlsname(nodename);
 * char *nodename;	nodename is a null terminated string
 *			note: uname(2) does not necessarily return a null
 *			terminated string.
 *
 * given a null terminated unix nodename, nlsname permutes the nodename and
 * returns a NULL terminated string which can be used to connect
 * to a listener process on 'nodename'.  If the input string + permute string
 * is longer than nlsname's static buffer length, nlsname returns a null ptr.
 * The name must be run through lname2addr before being posted to the network.
 * For remote login service, use lname2addr(nodename).
 *
 *	permuted_name = nlsname(name);
 *	name_length = strlen(pname);
 *
 * WARNING: The return value points to static data that are overwritten
 *	    by each call to this routine.
 *	    
 */

static char *permute = ".serve";
static char netname[NAMEBUFSZ + 1];

char *
nlsname(p)
register char *p;
{
	register char *np = netname;
	extern char *nlsn2addr();

	DEBUG((1,"nlsname: permuting %s, len %d", p, strlen(p)));

	if ( (strlen(p) + strlen(permute)) > NAMEBUFSZ )  {
		logmessage("nlsname: nodename arg too long");
		return((char *)0);
	}

	strncpy(np, p, NAMEBUFSZ);
	strncat(np, permute, NAMEBUFSZ);
	*(np + strlen(np)) = (char)0;

	DEBUG((1, "nlsname: returning %s, length %d", np, strlen(np)));

	return(np);
}
