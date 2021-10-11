#ifndef lint
static	char sccsid[] = "@(#)_errlst.c 1.1 92/07/30 SMI";
#endif

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
#ident	"@(#)libnsl:nsl/_errlst.c	1.2"
*/

/*
 * transport errno
 */
int t_errno = 0;

int t_nerr = 19;

int _dummy_errno[4] = {0, 0, 0, 0};
int _dummy_nerr[4] = {0, 0, 0, 0};
char **_dummy_errlst[4] = {0, 0, 0, 0};



/* 
 * transport interface error list
 */

char *t_errlist[] = {
	"No Error",					/*  0 */
	"Incorrect address format",		  	/*  1 */
	"Incorrect options format",			/*  2 */
	"Illegal permmissions",				/*  3 */
	"Illegal file descriptor",			/*  4 */
	"Couldn't allocate address",  			/*  5 */
	"Routine will place interface out of state",    /*  6 */
	"Illegal called/calling sequence number",	/*  7 */
	"System error",					/*  8 */
	"An event requires attention",			/*  9 */
	"Illegal amount of data",			/* 10 */
	"Buffer not large enough",			/* 11 */
	"Can't send message - (blocked)",		/* 12 */
	"No message currently available",		/* 13 */
	"Disconnect message not found",			/* 14 */
	"Unitdata error message not found",		/* 15 */
	"Incorrect flags specified",			/* 16 */
	"Orderly release message not found",            /* 17 */
	"Primitive not supported by provider",		/* 18 */
	"State is in process of changing",              /* 19 */
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	""
};
