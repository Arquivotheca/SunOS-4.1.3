
/*	@(#)ns_getaddr.c 1.1 92/07/30 SMI 	*/

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* #ident	"@(#)libns:ns_getaddr.c	1.4" */
#include <stdio.h>
#include <tiuser.h>
#include <rfs/nsaddr.h>
#include <rfs/nserve.h>

struct address *
ns_getaddr(resource, ro_flag, dname)
char	 *resource;
int	  ro_flag;
char	 *dname;
{
	struct nssend send;
	struct nssend *rtn;
	struct nssend *ns_getblock();

	/*
	 *	Initialize the information structure to send to the
	 *	name server.
	 */

	send.ns_code = NS_QUERY;
	send.ns_flag = ro_flag;
	send.ns_name = resource;
	send.ns_type = 0;
	send.ns_desc = NULL;
	send.ns_path = NULL;
	send.ns_addr = NULL;
	send.ns_mach = NULL;

	/*
	 *	start up communication with name server.
	 */

	if ((rtn = ns_getblock(&send)) == (struct nssend *)NULL)
		return((struct address *)NULL);

	if (dname)
		strncpy(dname,rtn->ns_mach[0],MAXDNAME);
	return(rtn->ns_addr);
}
