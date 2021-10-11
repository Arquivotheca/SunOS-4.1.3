
/*	@(#)ns_info.c 1.1 92/07/30 SMI 	*/

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*  #ident	"@(#)libns:ns_info.c	1.9" */
#include  <stdio.h>
#include  <rfs/nserve.h>
#include  <tiuser.h>
#include  <rfs/nsaddr.h>
#include  "stdns.h"
#include  "nsdb.h"

extern int ns_errno;

ns_info(name)
char	*name;
{

	static	char	*flg[] = {
		"read/write",
		"read-only"
	};
	char	dname[SZ_DELEMENT];
	struct	nssend 	send, *rtn;


	if (name[strlen(name)-1] == SEPARATOR) {
		sprintf(dname,"%s%c",name,WILDCARD);
		name = dname;
		send.ns_code = NS_QUERY;
	}
	else if (*name == WILDCARD)
		send.ns_code = NS_QUERY;
	else
		send.ns_code = NS_BYMACHINE;

	send.ns_type = 0;
	send.ns_flag = 0;
	send.ns_name = name;
	send.ns_path = NULL;
	send.ns_desc = NULL;
	send.ns_mach = NULL;

	/*
	 *	Setup communication path to the name server.
	 */
	
	if (ns_setup() == RFS_FAILURE)
		return(RFS_FAILURE);
	
	if (ns_send(&send) == RFS_FAILURE) {
		ns_close();
		return(RFS_FAILURE);
	}

	if ((rtn = ns_rcv()) == NULL) {
		ns_close();
		return(RFS_FAILURE);
	}
	
	do {
		if (rtn->ns_code == RFS_FAILURE) {
			if (ns_errno == R_NONAME)
				break;
			ns_close();
			return(RFS_FAILURE);
		}

		fprintf(stdout,"%-14.14s  %-10s  %-24.24s  %s\n",
				rtn->ns_name,flg[rtn->ns_flag],
				*rtn->ns_mach,
				(rtn->ns_desc) ? rtn->ns_desc : " "); 

		if (!(rtn->ns_code & MORE_DATA))
			break;

	} while ((rtn = ns_rcv()) != NULL);

	ns_close();
	return(RFS_SUCCESS);
}
