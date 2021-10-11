
/*	@(#)ns_getblock.c 1.1 92/07/30 SMI 	*/

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* #ident	"@(#)libns:ns_getblock.c	1.3" */
#include <stdio.h>
#include <tiuser.h>
#include <rfs/nsaddr.h>
#include <rfs/nserve.h>

struct nssend *
ns_getblock(send)
struct nssend *send;
{
	struct nssend *rcv;

	/*
	 *	Setup the communication path to the name server.
	 */
	
	if (ns_setup() == RFS_FAILURE)
		return((struct nssend *)NULL);
	
	if (ns_send(send) == RFS_FAILURE) {
		ns_close();
		return((struct nssend *)NULL);
	}

	/*
	 *	Get a return structure and check the return code
	 *	from the name server.
	 */
	
	if ((rcv = ns_rcv()) == (struct nssend *)NULL) {
		ns_close();
		return((struct nssend *)NULL);
	}

	if (rcv->ns_code != RFS_SUCCESS) {
		ns_close();
		return((struct nssend *)NULL);
	}

	ns_close();

	return(rcv);
}
