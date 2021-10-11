
/*	@(#)ns_verify.c 1.1 92/07/30 SMI 	*/

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* #ident	"@(#)libns:ns_verify.c	1.4" */
#include <stdio.h>
#include <tiuser.h>
#include <rfs/nsaddr.h>
#include <rfs/nserve.h>
#include "stdns.h"
#include "nsdb.h"

char *
ns_verify(name, passwd)
char *name, *passwd;
{
	struct nssend send;
	struct nssend *rtn;
	struct nssend *ns_getblock();

	/*
	 *	Initialize the information structure to send to the
	 *	name server.
	 */

	send.ns_code = NS_VERIFY;
	send.ns_type = 0;
	send.ns_flag = 0;
	send.ns_name = name;
	send.ns_desc = passwd;
	send.ns_mach = NULL;
	send.ns_addr = NULL;
	send.ns_path = NULL;

	while ((rtn = ns_getblock(&send)) == (struct nssend *)NULL &&
		ns_errno == R_INREC)
			sleep(1);

	if (rtn == (struct nssend *) NULL)
		return((char *)NULL);

	return(rtn->ns_desc);
}
