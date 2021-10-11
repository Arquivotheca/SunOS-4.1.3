#if !defined(lint) && defined(SCCSIDS)
static char sccsid[] = "@(#)svc_raw.c 1.1 92/07/30 Copyr 1984 Sun Micro";
#endif

/*
 * svc_raw.c,   This a toy for simple testing and timing.
 * Interface to create an rpc client and server in the same UNIX process.
 * This lets us similate rpc and get rpc (round trip) overhead, without
 * any interference from the kernal.
 *
 * Copyright (C) 1984, Sun Microsystems, Inc.
 */

#include <rpc/rpc.h>
#include <rpc/raw.h>

/*
 * This is the "network" that we will be moving data over
 */
static struct svcraw_private {
	char	*raw_buf;
	SVCXPRT	server;
	XDR	xdr_stream;
	char	verf_body[MAX_AUTH_BYTES];
} *svcraw_private;

extern char	*_rpcrawcombuf;

static struct xp_ops *svcraw_ops();

SVCXPRT *
svcraw_create()
{
	register struct svcraw_private *srp = svcraw_private;

	if (srp == NULL) {
		srp = (struct svcraw_private *)calloc(1, sizeof (*srp));
		if (srp == NULL)
			return (NULL);
		if (_rpcrawcombuf == NULL) {
			_rpcrawcombuf = (char *)calloc(UDPMSGSIZE,
							sizeof (char));
			if (_rpcrawcombuf == NULL)
				return (NULL);
		}
		srp->raw_buf = _rpcrawcombuf; /* Share it with the client */
		svcraw_private = srp;
	}
	srp->server.xp_sock = 0;
	srp->server.xp_port = 0;
	srp->server.xp_p3 = NULL;
	srp->server.xp_ops = svcraw_ops();
	srp->server.xp_verf.oa_base = srp->verf_body;
	xdrmem_create(&srp->xdr_stream, srp->raw_buf, UDPMSGSIZE, XDR_FREE);
	xprt_register(&srp->server);
	return (&srp->server);
}

static enum xprt_stat
svcraw_stat()
{

	return (XPRT_IDLE);
}

static bool_t
svcraw_recv(xprt, msg)
	SVCXPRT *xprt;
	struct rpc_msg *msg;
{
	register struct svcraw_private *srp = svcraw_private;
	register XDR *xdrs;

	if (srp == 0)
		return (0);
	xdrs = &srp->xdr_stream;
	xdrs->x_op = XDR_DECODE;
	XDR_SETPOS(xdrs, 0);
	if (! xdr_callmsg(xdrs, msg))
		return (FALSE);
	return (TRUE);
}

static bool_t
svcraw_reply(xprt, msg)
	SVCXPRT *xprt;
	struct rpc_msg *msg;
{
	register struct svcraw_private *srp = svcraw_private;
	register XDR *xdrs;

	if (srp == 0)
		return (FALSE);
	xdrs = &srp->xdr_stream;
	xdrs->x_op = XDR_ENCODE;
	XDR_SETPOS(xdrs, 0);
	if (! xdr_replymsg(xdrs, msg))
		return (FALSE);
	(void)XDR_GETPOS(xdrs);  /* called just for overhead */
	return (TRUE);
}

static bool_t
svcraw_getargs(xprt, xdr_args, args_ptr)
	SVCXPRT *xprt;
	xdrproc_t xdr_args;
	caddr_t args_ptr;
{
	register struct svcraw_private *srp = svcraw_private;

	if (srp == 0)
		return (FALSE);
	return ((*xdr_args)(&srp->xdr_stream, args_ptr));
}

static bool_t
svcraw_freeargs(xprt, xdr_args, args_ptr)
	SVCXPRT *xprt;
	xdrproc_t xdr_args;
	caddr_t args_ptr;
{
	register struct svcraw_private *srp = svcraw_private;
	register XDR *xdrs;

	if (srp == 0)
		return (FALSE);
	xdrs = &srp->xdr_stream;
	xdrs->x_op = XDR_FREE;
	return ((*xdr_args)(xdrs, args_ptr));
}

static void
svcraw_destroy()
{
}

static struct xp_ops *
svcraw_ops()
{
	static struct xp_ops ops;

	if (ops.xp_recv == NULL) {
		ops.xp_recv = svcraw_recv;
		ops.xp_stat = svcraw_stat;
		ops.xp_getargs = svcraw_getargs;
		ops.xp_reply = svcraw_reply;
		ops.xp_freeargs = svcraw_freeargs;
		ops.xp_destroy = svcraw_destroy;
	}
	return (&ops);
}
