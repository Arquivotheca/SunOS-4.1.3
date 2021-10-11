
/*	@(#)ns_findp.c 1.1 92/07/30 SMI 	*/

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* #ident	"@(#)libns:ns_findp.c	1.2" */
#include <stdio.h>
#include <tiuser.h>
#include <rfs/nsaddr.h>
#include <rfs/nserve.h>

char *
ns_findp(dname)
char	 *dname;
{
	struct nssend send;
	struct nssend *rtn;
	struct nssend *ns_getblock();

	/*
	 *	Initialize the information structure to send to the
	 *	name server.
	 */

	send.ns_code = NS_FINDP;
	send.ns_flag = 0;
	send.ns_name = dname;
	send.ns_type = 0;
	send.ns_desc = NULL;
	send.ns_path = NULL;
	send.ns_addr = NULL;
	send.ns_mach = NULL;

	/*
	 *	start up communication with name server.
	 */

	if ((rtn = ns_getblock(&send)) == (struct nssend *)NULL)
		return((char *)NULL);

	return(rtn->ns_mach[0]);
}
