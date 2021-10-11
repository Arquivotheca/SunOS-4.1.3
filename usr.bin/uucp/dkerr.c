/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident  "@(#)dkerr.c 1.1 92/07/30"	/* from SVR3.2 uucp:dkerr.c 1.1 */

/*
 *	Convert an error number from the Common Control into a string
 */
	static char	SCCSID[] = "@(#)dkerr.c	2.6+BNU DKHOST 87/03/05";
/*
 *	COMMKIT(TM) Software - Datakit(R) VCS Interface Release 2.0 V1
 *			Copyright 1984 AT&T
 *			All Rights Reserved
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
 *     The copyright notice above does not evidence any actual
 *          or intended publication of such source code.
 */

#include "sysexits.h"

char *dk_msgs[] = {
	"Call Failed",		/* code 0 - Something is Wrong */
	"All channels busy",	/* code 1 - busy */
	"Remote node not answering",	/* code 2 - trunk down */
	"Server not answering",	/* code 3 - termporary no dest */
	"Non-assigned number",	/* code 4 - permonent no dest (INTERT) */
	"All trunk channels busy",	/* code 5 - System Overload (REORT) */
	"Server already exists",	/* code 6 - already exists */
	"Access denied",		/* code 7 - denied by remote server */
	"",				/* code 8 - directory assistance req */
} ;

char 	*dialer_msgs[] = {
	"",
	"Please supply a valid phone number",	/* code 1 - phone # missing */
	"No response from auto-dialer. Try again",	/* code 2- bad port */
	"Auto dialer failed to initiate call. Try again", /* code 3 - dial failure */
	"No initial dial tone detected",	/* code 4 - bad telephone line */
	"No secondary dial tone detected",	/* code 5 - no sec. dial tone */
	"Dialed number is busy",	/* code 6 - busy signal detected */
	"No answer fron dialed number",	/* code 7 - auto-dialer didn't get ans. */
	"No carrier tone was detected",	/* code 8 - no carrier tone det. */
	"Could not complete your call. Try again.",	/* code 9 - auto dialer didn't complete */
	"Wrong number", /*code 10 - bad number*/
};

char *dk_hostmsgs[] = {
	"Dkserver: Can't open line: See System Administrator", /* Code 130 */
	"", /* Code 131 */
	"", /* Code 132 */
	"Dkserver: Dksrvtab not readable: See System Administrator",		/* Code 133 */
	"Dkserver: Can't chroot: See System Administrator",
};

#define NDKMSGS	(sizeof(dk_msgs)/sizeof(dk_msgs[0]))
#define NDKHOMSGS	(sizeof(dk_hostmsgs)/sizeof(dk_hostmsgs[0]))
#define DIALERCODE	11

	int	dk_verbose = 1;	/* Print error messages on stderr if 1 */
	int	dk_errno = 0;	/* Saved error number from iocb.req_error */

	static char	generalmsg[32];

	char *
dkerr(err)
{

	if ((err & 0377) == DIALERCODE)
		return(dialer_msgs[err>>8]);

	if ((err >= 0 && err <= 99) && err < NDKMSGS && dk_msgs[err] != 0)
		return(dk_msgs[err]);
	if ((err >= 100) && (err < (NDKHOMSGS + 130)) && (dk_hostmsgs[err-130] != 0)) 
		return(dk_hostmsgs[err-130]);
	sprintf(generalmsg, "Error code %d", err) ;
	return(generalmsg);
}

dkerrmap(dkcode)
{
	if (dkcode < 0)
		return(-dkcode);

	switch(dkcode){
	case 0:
	case 1:
	case 2:
	case 3:
	case 5:
		return(EX_TEMPFAIL);

	case 4:
		return(EX_NOHOST);

	case 6:
		return(EX_CANTCREAT);

	case 7:
		return(EX_NOPERM);

	default:
		return(EX_DATAERR);
	}
}
