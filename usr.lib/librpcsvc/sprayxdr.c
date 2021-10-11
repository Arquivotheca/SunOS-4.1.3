#ifndef lint
static  char sccsid[] = "@(#)sprayxdr.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include <sys/time.h>
#include <rpc/rpc.h>
#include <rpcsvc/spray.h>

xdr_sprayarr(xdrsp, arrp)
	XDR *xdrsp;
	struct sprayarr *arrp;
{
	if (!xdr_bytes(xdrsp, (char **) &arrp->data, &arrp->lnth, SPRAYMAX))
		return(0);
	return(1);
}

xdr_spraycumul(xdrsp, cumulp)
	XDR *xdrsp;
	struct spraycumul *cumulp;
{
	if (!xdr_u_int(xdrsp, &cumulp->counter))
		return(0);
	if (!xdr_timeval(xdrsp, &cumulp->clock))
		return(0);
	return(1);
}
