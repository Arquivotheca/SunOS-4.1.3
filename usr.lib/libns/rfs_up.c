/*	@(#)rfs_up.c 1.1 92/07/30 SMI 	*/

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* #ident  "@(#)libns:rfs_up.c     1.1.1.1" */
#include <rfs/rfsys.h>
#include <errno.h>
#include <rfs/nserve.h>

rfs_up()
{
	char dname[MAXDNAME];


	/*
	 *	Determine if RFS is running by first obtaining the
	 *	domain name and then trying to push it into the
	 *	kernel.
	 */

	if (rfsys(RF_GETDNAME, dname, MAXDNAME) < 0) {
		errno=ENONET;
		return(-1);
	}
	if (rfsys(RF_SETDNAME, dname, strlen(dname)+1) >= 0) {
		errno=ENONET;
		return(-1);
	}
	return(0);
}
