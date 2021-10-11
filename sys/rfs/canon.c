/*	@(#)canon.c 1.1 92/07/30 SMI 	*/

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* #ident	"@(#)libns:canon.c	1.3" */

/*
 * Wrappers for rfs XDR conversion routines. These routines are called using
 * the tcanon() and fcanon() macros #defined in rfs_xdr.h
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/vnode.h>
#include <sys/kmem_alloc.h>
#include <rpc/rpc.h>
#include <rfs/rfs_misc.h>
#include <rfs/message.h>
#include <rfs/sema.h>
#include <rfs/comm.h>
#include <rfs/rfs_xdr.h>


u_int
xdrwrap_rfs_flock(from, to, dir)
register char *from, *to;
enum xdr_op dir;			/* Encode or decode */
{
	char *tp;
	bool_t rs;
	XDR xdrs;
	char tbuf[XR_LOCK];
	u_int xdrcount = 0;

	if (from == to)
		tp = tbuf;
	else
		tp = to;

	xdrmem_create(&xdrs, (dir==XDR_ENCODE ? tp : from), (u_int) XR_LOCK, dir);

	rs = xdr_rfs_flock(&xdrs, (rfs_flock *) (dir==XDR_ENCODE ? from : tp));

	if (rs == TRUE) {
		xdrcount = xdr_getpos(&xdrs);
		if (from == to)
			bcopy(tp, to, xdrcount);
	}
	xdr_destroy(&xdrs);
	return (xdrcount);
}


u_int
xdrwrap_rfs_stat(from, to, dir)
register char *from, *to;
enum xdr_op dir;			/* Encode or decode */
{
	char *tp;
	bool_t rs;
	XDR xdrs;
	char tbuf[XR_STAT];
	u_int xdrcount = 0;

	if (from == to)
		tp = tbuf;
	else
		tp = to;

	xdrmem_create(&xdrs, (dir==XDR_ENCODE ? tp : from), (u_int) XR_STAT, dir);

	rs = xdr_rfs_stat(&xdrs, (rfs_stat *) (dir==XDR_ENCODE ? from : tp));

	if (rs == TRUE) {
		xdrcount = xdr_getpos(&xdrs);
		if (from == to)
			bcopy(tp, to, xdrcount);
	}
	xdr_destroy(&xdrs);
	return (xdrcount);
}

u_int
xdrwrap_rfs_statfs(from, to, dir)
register char *from, *to;
enum xdr_op dir;			/* Encode or decode */
{
	char *tp;
	bool_t rs;
	XDR xdrs;
	char tbuf[XR_STATFS];
	u_int xdrcount = 0;

	if (from == to)
		tp = tbuf;
	else
		tp = to;

	xdrmem_create(&xdrs, (dir==XDR_ENCODE ? tp : from), (u_int) XR_STATFS, dir);

	rs = xdr_rfs_statfs(&xdrs, (rfs_statfs *) (dir==XDR_ENCODE ? from : tp));

	if (rs == TRUE) {
		xdrcount = xdr_getpos(&xdrs);
		if (from == to)
			bcopy(tp, to, xdrcount);
	}
	xdr_destroy(&xdrs);
	return (xdrcount);
}

u_int
xdrwrap_rfs_ustat(from, to, dir)
register char *from, *to;
enum xdr_op dir;			/* Encode or decode */
{
	char *tp;
	bool_t rs;
	XDR xdrs;
	char tbuf[XR_USTAT];
	u_int xdrcount = 0;

	if (from == to)
		tp = tbuf;
	else
		tp = to;

	xdrmem_create(&xdrs, (dir==XDR_ENCODE ? tp : from), (u_int) XR_USTAT, dir);

	rs = xdr_rfs_ustat(&xdrs, (struct rfs_ustat *) (dir==XDR_ENCODE ? from : tp));

	if (rs == TRUE) {
		xdrcount = xdr_getpos(&xdrs);
		if (from == to)
			bcopy(tp, to, xdrcount);
	}
	xdr_destroy(&xdrs);
	return (xdrcount);
}

u_int
xdrwrap_rfs_utimbuf(from, to, dir)
register char *from, *to;
enum xdr_op dir;			/* Encode or decode */
{
	char *tp;
	bool_t rs;
	XDR xdrs;
	char tbuf[XR_USTAT];
	u_int xdrcount = 0;

	if (from == to)
		tp = tbuf;
	else
		tp = to;

	xdrmem_create(&xdrs, (dir==XDR_ENCODE ? tp : from), (u_int) XR_UTIMBUF, dir);

	rs = xdr_rfs_utimbuf(&xdrs, (struct rfs_utimbuf *) (dir==XDR_ENCODE ? from : tp));

	if (rs == TRUE) {
		xdrcount = xdr_getpos(&xdrs);
		if (from == to)
			bcopy(tp, to, xdrcount);
	}
	xdr_destroy(&xdrs);
	return (xdrcount);
}

u_int
xdrwrap_rfs_dirent(from, to, dir)
register char *from, *to;
enum xdr_op dir;			/* Encode or decode */
{
	char *tp;
	bool_t rs;
	XDR xdrs;
	u_int xdrcount = 0;
	u_int numbytes =
	    (RFS_MAXNAMLEN + 2 * sizeof (struct rfs_dirent) + 20 /* slop */);


	tp = (from == to) ? new_kmem_alloc(numbytes, KMEM_SLEEP) : to;
	xdrmem_create(&xdrs, (dir==XDR_ENCODE ? tp : from), numbytes, dir);

	rs = xdr_rfs_dirent(&xdrs, (rfs_dirent *) (dir==XDR_ENCODE ? from : tp));

	if (rs == TRUE) {
		xdrcount = xdr_getpos(&xdrs);
		if (from == to)
			bcopy(tp, to, xdrcount);
	}
	xdr_destroy(&xdrs);
	if (from == to)
		kmem_free(tp, numbytes);
	return (xdrcount);
}


u_int
xdrwrap_rfs_message(from, to, dir)
register char *from, *to;
enum xdr_op dir;			/* Encode or decode */
{
	char *tp;
	bool_t rs;
	XDR xdrs;
	char tbuf[XR_MESSAGE];
	u_int xdrcount = 0;

	if (from == to)
		tp = tbuf;
	else
		tp = to;

	xdrmem_create(&xdrs, (dir==XDR_ENCODE ? tp : from), (u_int) XR_MESSAGE, dir);

	rs = xdr_rfs_message(&xdrs, (struct message *) (dir==XDR_ENCODE ? from : tp));

	if (rs == TRUE) {
		xdrcount = xdr_getpos(&xdrs);
		if (from == to)
			bcopy(tp, to, xdrcount);
	}
	xdr_destroy(&xdrs);
	return (xdrcount);
}


u_int
xdrwrap_rfs_request(from, to, dir)
register char *from, *to;
enum xdr_op dir;			/* Encode or decode */
{
	char *tp;
	bool_t rs;
	XDR xdrs;
	char tbuf[XR_REQ];
	u_int xdrcount = 0;

	if (from == to)
		tp = tbuf;
	else
		tp = to;

	xdrmem_create(&xdrs, (dir==XDR_ENCODE ? tp : from), (u_int) XR_REQ, dir);

	rs = xdr_rfs_request(&xdrs, (struct request *) (dir==XDR_ENCODE ? from : tp));

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
enum xdr_op dir;			/* Encode or decode */
{
	char *tp;
	bool_t rs;
	XDR xdrs;
	u_int xdrcount = 0;
	u_int numbytes = DATASIZE;

	tp = (from == to) ? new_kmem_alloc(numbytes, KMEM_SLEEP) : to;
	xdrmem_create(&xdrs, (dir==XDR_ENCODE ? tp : from), numbytes, dir);

	rs = xdr_rfs_string(&xdrs, (dir==XDR_ENCODE ? from : tp));

	if (rs == TRUE) {
		xdrcount = xdr_getpos(&xdrs);
		if (from == to)
			bcopy(tp, to, xdrcount);
	}
	xdr_destroy(&xdrs);
	if (from == to)
		kmem_free(tp, numbytes);
	return (xdrcount);
}
