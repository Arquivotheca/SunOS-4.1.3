/*	@(#)canon.c 1.1 92/07/30 SMI 	*/

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libns:canon.c	1.3.2.1"

/*
 * Wrappers for rfs XDR conversion routines. These routines are called using
 * the tcanon() and fcanon() macros #defined in ns_xdr.h
 */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/mount.h>
#include <rfs/message.h>
#include <rpc/rpc.h>
#include <rfs/ns_xdr.h>
#include <rfs/nserve.h>
#include <rfs/cirmgr.h>
#include <rfs/pn.h>
#include "nsports.h"

u_int
xdrwrap_rfs_n_data(from, to, dir)
register char *from, *to;
enum	xdr_op dir;			/* Encode or decode */
{
	char *tp;
	bool_t rs;
	XDR xdrs;
	char tbuf[XR_NDATA];
	u_int xdrcount = 0;

	if (from == to)
		tp = tbuf;
	else
		tp = to;

	xdrmem_create(&xdrs, (dir==XDR_ENCODE ? tp : from), (u_int) XR_NDATA, dir);

	rs = xdr_rfs_n_data(&xdrs, (struct n_data *) (dir==XDR_ENCODE ? from : tp));

	if (rs == TRUE) {
		xdrcount = xdr_getpos(&xdrs);
		if (from == to)
			bcopy(tp, to, xdrcount);
	}
	xdr_destroy(&xdrs);
	return (xdrcount);
}


u_int
xdrwrap_rfs_pkt_hd(from, to, dir)
register char *from, *to;
enum	xdr_op dir;			/* Encode or decode */
{
	char *tp;
	bool_t rs;
	XDR xdrs;
	char tbuf[XR_PKTHD];
	u_int xdrcount = 0;

	if (from == to)
		tp = tbuf;
	else
		tp = to;

	xdrmem_create(&xdrs, (dir==XDR_ENCODE ? tp : from), (u_int) XR_PKTHD, dir);

	rs = xdr_rfs_pkt_hd(&xdrs, (struct pkt_hd *) (dir==XDR_ENCODE ? from : tp));

	if (rs == TRUE) {
		xdrcount = xdr_getpos(&xdrs);
		if (from == to)
			bcopy(tp, to, xdrcount);
	}
	xdr_destroy(&xdrs);
	return (xdrcount);
}


u_int
xdrwrap_rfs_first_msg(from, to, dir)
register char *from, *to;
enum	xdr_op dir;			/* Encode or decode */
{
	char *tp;
	bool_t rs;
	XDR xdrs;
	char tbuf[XR_FIRSTMSG];
	u_int xdrcount = 0;

	if (from == to)
		tp = tbuf;
	else
		tp = to;

	xdrmem_create(&xdrs, (dir==XDR_ENCODE ? tp : from), (u_int) XR_FIRSTMSG, dir);

	rs = xdr_rfs_first_msg(&xdrs, (struct first_msg *) (dir==XDR_ENCODE ? from : tp));

	if (rs == TRUE) {
		xdrcount = xdr_getpos(&xdrs);
		if (from == to)
			bcopy(tp, to, xdrcount);
	}
	xdr_destroy(&xdrs);
	return (xdrcount);
}


u_int
xdrwrap_rfs_pnhdr(from, to, dir)
register char *from, *to;
enum	xdr_op dir;			/* Encode or decode */
{
	char *tp;
	bool_t rs;
	XDR xdrs;
	char tbuf[XR_PNHDR];
	u_int xdrcount = 0;

	if (from == to)
		tp = tbuf;
	else
		tp = to;

	xdrmem_create(&xdrs, (dir==XDR_ENCODE ? tp : from), (u_int) XR_PNHDR, dir);

	rs = xdr_rfs_pnhdr(&xdrs, (pnhdr_t *) (dir==XDR_ENCODE ? from : tp));

	if (rs == TRUE) {
		xdrcount = xdr_getpos(&xdrs);
		if (from == to)
			bcopy(tp, to, xdrcount);
	}
	xdr_destroy(&xdrs);
	return (xdrcount);
}


u_int
xdrwrap_rfs_long(from, to, dir)
register char *from, *to;
enum	xdr_op dir;			/* Encode or decode */
{
	char *tp;
	bool_t rs;
	XDR xdrs;
	char tbuf[XR_LONG];
	u_int xdrcount = 0;

	if (from == to)
		tp = tbuf;
	else
		tp = to;

	xdrmem_create(&xdrs, (dir==XDR_ENCODE ? tp : from), (u_int) XR_LONG, dir);

	rs = xdr_long(&xdrs, (long *) (dir==XDR_ENCODE ? from : tp));

	if (rs == TRUE) {
		xdrcount = xdr_getpos(&xdrs);
		if (from == to)
			bcopy(tp, to, xdrcount);
	}
	xdr_destroy(&xdrs);
	return (xdrcount);
}


u_int
xdrwrap_rfs_string(from, to, dir)
register char *from, *to;
enum	xdr_op dir;			/* Encode or decode */
{
	char *tp;
	bool_t rs;
	XDR xdrs;
	char tbuf[XR_STRING];
	u_int xdrcount = 0;

	if (from == to)
		tp = tbuf;
	else
		tp = to;

	xdrmem_create(&xdrs, (dir==XDR_ENCODE ? tp : from), (u_int) XR_STRING, dir);

	rs = xdr_rfs_string(&xdrs, (char *) (dir==XDR_ENCODE ? from : tp));

	if (rs == TRUE) {
		xdrcount = xdr_getpos(&xdrs);
		if (from == to)
			bcopy(tp, to, xdrcount);
	}
	xdr_destroy(&xdrs);
	return (xdrcount);
}
