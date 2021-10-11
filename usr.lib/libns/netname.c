/*	@(#)netname.c 1.1 92/07/30 SMI 	*/

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* #ident	"@(#)libns:netname.c	1.3" */
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <rfs/nserve.h>
#include <rfs/rfsys.h>

int
netname(s)
char *s;
{
	char nodename[SZ_MACH];

/*
 *	flow: 
 *
 *	1) make a call to rfsys to get domain name
 *	2) do a uname to get sysname 
 *	3) concatonate with "."
 *	4) return netnodename
 *
 */

	if(rfsys(RF_GETDNAME, s, MAXDNAME) < 0) {
		perror("netname");
		return(-1);
	}
	if(gethostname(nodename, SZ_MACH - 1) < 0) {
		perror("netname");
		return(-1);
	}
	nodename[SZ_MACH - 1] = '\0';
	if (strlen(s) + strlen(nodename) > MAXDNAME) {
		fprintf(stderr, "netname: name too long (> %d)\n",
			MAXDNAME);
		return(-1);
	}

	(void)sprintf(s + strlen(s),"%c%s",SEPARATOR, nodename);
	return(0);
}
