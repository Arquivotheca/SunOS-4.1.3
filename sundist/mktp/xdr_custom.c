#ifndef lint
static  char sccsid[] = "@(#)xdr_custom.c 1.1 92/07/30 Copyr 1987 Sun Micro";
#endif
/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 *
 */

#include "toc.h"

bool_t
xdr_futureslop(xdrs, objp)
XDR *xdrs;
struct futureslop *objp;
{
#ifdef	lint
	objp = objp;
#endif
	if(xdrs->x_op == XDR_DECODE)
		return(xdrrec_skiprecord(xdrs));
	(void) fprintf(stderr,"error in xdr_futureslop\n");
	return(FALSE);
}

bool_t
xdr_nomedia(xdrs, objp)
XDR *xdrs;
struct nomedia *objp;
{
#ifdef	lint
	xdrs = xdrs;
	objp = objp;
#endif
	(void) fprintf(stderr,"xdr_nomedia- No such media supported\n");
	return(FALSE);
}
