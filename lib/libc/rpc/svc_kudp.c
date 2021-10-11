#ifdef	KERNEL
#ident	"@(#)svc_kudp.c 1.1 92/07/30 Copyr 1984 Sun Micro"

/*
 * svc_kudp.c,
 * Server side for UDP/IP based RPC in the kernel.
 *
 * Copyright (C) 1984, Sun Microsystems, Inc.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <rpc/types.h>
#include <netinet/in.h>
#include <rpc/xdr.h>
#include <rpc/auth.h>
#include <rpc/clnt.h>
#include <rpc/rpc_msg.h>
#include <rpc/svc.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/mbuf.h>
#include <sys/vfs.h>
#include <sys/vnode.h>
#include <sys/errno.h>
#include <nfs/nfs.h>

/*
 * states of requests for duplicate request caching
 */
#define	DUP_INPROGRESS		0x01	/* request already going */
#define	DUP_DONE		0x02	/* request done */
#define	DUP_DROP		0x03	/* drop the request */

#define	rpc_buffer(xprt) ((xprt)->xp_p1)

/*
 * Routines exported through ops vector.
 */
bool_t		svckudp_recv();
bool_t		svckudp_send();
enum xprt_stat	svckudp_stat();
bool_t		svckudp_getargs();
bool_t		svckudp_freeargs();
void		svckudp_destroy();

/*
 * Server transport operations vector.
 */
struct xp_ops svckudp_op = {
	svckudp_recv,		/* Get requests */
	svckudp_stat,		/* Return status */
	svckudp_getargs,	/* Deserialize arguments */
	svckudp_send,		/* Send reply */
	svckudp_freeargs,	/* Free argument data space */
	svckudp_destroy		/* Destroy transport handle */
};


struct mbuf	*ku_recvfrom();
void		xdrmbuf_init();

/*
 * Transport private data.
 * Kept in xprt->xp_p2.
 */
struct udp_data {
	int	ud_flags;			/* flag bits, see below */
	u_long 	ud_xid;				/* id */
	struct	mbuf *ud_inmbuf;		/* input mbuf chain */
	XDR	ud_xdrin;			/* input xdr stream */
	XDR	ud_xdrout;			/* output xdr stream */
	char	ud_verfbody[MAX_AUTH_BYTES];	/* verifier */
};


/*
 * Flags
 */
#define	UD_BUSY		0x001		/* buffer is busy */
#define	UD_WANTED	0x002		/* buffer wanted */

/*
 * Server statistics
 */
struct {
	int	rscalls;
	int	rsbadcalls;
	int	rsnullrecv;
	int	rsbadlen;
	int	rsxdrcall;
} rsstat;

/*
 * Create a transport record.
 * The transport record, output buffer, and private data structure
 * are allocated.  The output buffer is serialized into using xdrmem.
 * There is one transport record per user process which implements a
 * set of services.
 */
SVCXPRT *
svckudp_create(sock, port)
	struct socket	*sock;
	u_short		 port;
{
	register SVCXPRT	 *xprt;
	register struct udp_data *ud;

#ifdef RPCDEBUG
	rpc_debug(4, "svckudp_create so = %x, port = %d\n", sock, port);
#endif
	xprt = (SVCXPRT *)kmem_alloc((u_int)sizeof (SVCXPRT));
	rpc_buffer(xprt) = (caddr_t)kmem_alloc((u_int)UDPMSGSIZE);
	ud = (struct udp_data *)kmem_alloc((u_int)sizeof (struct udp_data));
	bzero((caddr_t)ud, sizeof (*ud));
	xprt->xp_addrlen = 0;
	xprt->xp_p2 = (caddr_t)ud;
	xprt->xp_p3 = NULL;
	xprt->xp_verf.oa_base = ud->ud_verfbody;
	xprt->xp_ops = &svckudp_op;
	xprt->xp_port = port;
	xprt->xp_sock = sock;
	xprt_register(xprt);
	return (xprt);
}

/*
 * Destroy a transport record.
 * Frees the space allocated for a transport record.
 */
void
svckudp_destroy(xprt)
	register SVCXPRT   *xprt;
{
	register struct udp_data *ud = (struct udp_data *)xprt->xp_p2;
	int						  s;

#ifdef RPCDEBUG
	rpc_debug(4, "usr_destroy %x\n", xprt);
#endif
	/*
	 * Make sure that the buffer is not in use before freeing it.
	 */
	s = splimp();
	while (ud->ud_flags & UD_BUSY) {
		ud->ud_flags |= UD_WANTED;
		(void) sleep((caddr_t)ud, PZERO-2);
	}
	(void) splx(s);

	if (ud->ud_inmbuf) {
		m_freem(ud->ud_inmbuf);
	}
	kmem_free((caddr_t)ud, (u_int)sizeof (struct udp_data));
	kmem_free((caddr_t)rpc_buffer(xprt), (u_int)UDPMSGSIZE);
	kmem_free((caddr_t)xprt, (u_int)sizeof (SVCXPRT));
}

/*
 * Receive rpc requests.
 * Pulls a request in off the socket, checks if the packet is intact,
 * and deserializes the call packet.
 */
bool_t
svckudp_recv(xprt, msg)
	register SVCXPRT	 *xprt;
	struct rpc_msg		 *msg;
{
	register struct udp_data *ud = (struct udp_data *)xprt->xp_p2;
	register XDR	 *xdrs = &(ud->ud_xdrin);
	register struct mbuf	 *m;
	int			  s;

#ifdef RPCDEBUG
	rpc_debug(4, "svckudp_recv %x\n", xprt);
#endif
	rsstat.rscalls++;
	s = splnet();
	m = ku_recvfrom(xprt->xp_sock, &(xprt->xp_raddr));
	(void) splx(s);
	if (m == NULL) {
		rsstat.rsnullrecv++;
		return (FALSE);
	}

	if (m->m_len < 4*sizeof (u_long)) {
		rsstat.rsbadlen++;
		goto bad;
	}
	xdrmbuf_init(&ud->ud_xdrin, m, XDR_DECODE);
	if (! xdr_callmsg(xdrs, msg)) {
		rsstat.rsxdrcall++;
		goto bad;
	}
	ud->ud_xid = msg->rm_xid;
	ud->ud_inmbuf = m;
#ifdef RPCDEBUG
	rpc_debug(5, "svckudp_recv done\n");
#endif
	return (TRUE);

bad:
	m_freem(m);
	ud->ud_inmbuf = NULL;
	rsstat.rsbadcalls++;
	return (FALSE);
}


static
buffree(ud)
	register struct udp_data *ud;
{
	ud->ud_flags &= ~UD_BUSY;
	if (ud->ud_flags & UD_WANTED) {
		ud->ud_flags &= ~UD_WANTED;
		wakeup((caddr_t)ud);
	}
}

/*
 * Send rpc reply.
 * Serialize the reply packet into the output buffer then
 * call ku_sendto to make an mbuf out of it and send it.
 */
bool_t
/* ARGSUSED */
svckudp_send(xprt, msg)
	register SVCXPRT *xprt;
	struct rpc_msg *msg;
{
	register struct udp_data *ud = (struct udp_data *)xprt->xp_p2;
	register XDR *xdrs = &(ud->ud_xdrout);
	register int slen;
	register int stat = FALSE;
	int s;
	struct mbuf *m, *mclgetx();

#ifdef RPCDEBUG
	rpc_debug(4, "svckudp_send %x\n", xprt);
#endif
	s = splimp();
	while (ud->ud_flags & UD_BUSY) {
		ud->ud_flags |= UD_WANTED;
		(void) sleep((caddr_t)ud, PZERO-2);
	}
	ud->ud_flags |= UD_BUSY;
	(void) splx(s);
	m = mclgetx(buffree, (int)ud, rpc_buffer(xprt), UDPMSGSIZE, M_WAIT);
	if (m == NULL) {
		buffree(ud);
		return (stat);
	}

	xdrmbuf_init(&ud->ud_xdrout, m, XDR_ENCODE);
	msg->rm_xid = ud->ud_xid;
	if (xdr_replymsg(xdrs, msg)) {
		slen = (int)XDR_GETPOS(xdrs);
		if (m->m_next == 0) {		/* XXX */
			m->m_len = slen;
		}
		if (!ku_sendto_mbuf(xprt->xp_sock, m, &xprt->xp_raddr))
			stat = TRUE;
	} else {
		printf("svckudp_send: xdr_replymsg failed\n");
		m_freem(m);
	}
	/*
	 * This is completely disgusting.  If public is set it is
	 * a pointer to a structure whose first field is the address
	 * of the function to free that structure and any related
	 * stuff.  (see rrokfree in nfs_xdr.c).
	 */
	if (xdrs->x_public) {
		(**((int (**)())xdrs->x_public))(xdrs->x_public);
	}
#ifdef RPCDEBUG
	rpc_debug(5, "svckudp_send done\n");
#endif
	return (stat);
}

/*
 * Return transport status.
 */
/*ARGSUSED*/
enum xprt_stat
svckudp_stat(xprt)
	SVCXPRT *xprt;
{

	return (XPRT_IDLE);
}

/*
 * Deserialize arguments.
 */
bool_t
svckudp_getargs(xprt, xdr_args, args_ptr)
	SVCXPRT	*xprt;
	xdrproc_t	 xdr_args;
	caddr_t		 args_ptr;
{

	return ((*xdr_args)(&(((struct udp_data *)(xprt->xp_p2))->ud_xdrin),
	    args_ptr));
}

bool_t
svckudp_freeargs(xprt, xdr_args, args_ptr)
	SVCXPRT	*xprt;
	xdrproc_t	 xdr_args;
	caddr_t		 args_ptr;
{
	register XDR *xdrs =
	    &(((struct udp_data *)(xprt->xp_p2))->ud_xdrin);
	register struct udp_data *ud = (struct udp_data *)xprt->xp_p2;

	if (ud->ud_inmbuf) {
		m_freem(ud->ud_inmbuf);
	}
	ud->ud_inmbuf = (struct mbuf *)0;
	if (args_ptr) {
		xdrs->x_op = XDR_FREE;
		return ((*xdr_args)(xdrs, args_ptr));
	} else {
		return (TRUE);
	}
}

/*
 * the dup cacheing routines below provide a cache of non-failure
 * transaction id's.  rpc service routines can use this to detect
 * retransmissions and re-send a non-failure response.
 */

struct dupreq {
	u_long		dr_xid;
	struct sockaddr_in dr_addr;
	u_long		dr_proc;
	u_long		dr_vers;
	u_long		dr_prog;
	caddr_t		dr_resp;	/* cached response	*/
	int		dr_status;	/* request status	*/
	struct dupreq	*dr_next;
	struct dupreq	*dr_chain;
};

/*
 * MAXDUPREQS is the number of cached items.  It should be adjusted
 * to the service load so that there is likely to be a response entry
 * when the first retransmission comes in.
 */
#define	MAXDUPREQS	400

#define	DUPREQSZ	(sizeof (struct dupreq) - 2*sizeof (caddr_t))
#define	DRHASHSZ	32
#define	XIDHASH(xid)	((xid) & (DRHASHSZ-1))
#define	DRHASH(dr)	XIDHASH((dr)->dr_xid)
#define	REQTOXID(req)	((struct udp_data *)((req)->rq_xprt->xp_p2))->ud_xid

int	ndupreqs;
int	dupreqs;
int	dupchecks;
struct dupreq *drhashtbl[DRHASHSZ];

/*
 * drmru points to the head of a circular linked list in lru order.
 * drmru->dr_next == drlru
 */
struct dupreq *drmru;

/*
 * scvkudp_dupsave searches the request cache and stores the
 * request.  If it already exists, then just mark it in progress.
 */
svckudp_dupsave(req, size)
	register struct svc_req *req;
	register int size;
{
	register struct dupreq *dr;

	if (ndupreqs < MAXDUPREQS) {
		dr = (struct dupreq *)kmem_alloc(sizeof (*dr));
		dr->dr_resp = (caddr_t)kmem_alloc((u_int)size);
		if (size && (dr->dr_resp == (caddr_t)0))
			panic("svckudp_dupsave: can't kmem_alloc");
		dr->dr_status = 0;
		if (drmru) {
			dr->dr_next = drmru->dr_next;
			drmru->dr_next = dr;
		} else {
			dr->dr_next = dr;
		}
		ndupreqs++;
	} else {
		dr = drmru->dr_next;
		unhash(dr);
	}
	drmru = dr;

	dr->dr_status = DUP_INPROGRESS;
	dr->dr_xid = REQTOXID(req);
	dr->dr_prog = req->rq_prog;
	dr->dr_vers = req->rq_vers;
	dr->dr_proc = req->rq_proc;
	dr->dr_addr = req->rq_xprt->xp_raddr;
	dr->dr_chain = drhashtbl[DRHASH(dr)];
	drhashtbl[DRHASH(dr)] = dr;
}

/*
 * svckudp_dup searches the request cache and returns 0 if the
 * request is not found in the cache.  If it is found, then it
 * returns the state of the request (in progress or done) and
 * the status or attributes that were part of the original reply.
 */
svckudp_dup(req, res, size)
	register struct svc_req *req;
	caddr_t	res;
	register int size;
{
	register struct dupreq *dr;
	u_long xid;

	dupchecks++;
	xid = REQTOXID(req);
	dr = drhashtbl[XIDHASH(xid)];
	while (dr != NULL) {
		if (dr->dr_xid != xid ||
		    dr->dr_prog != req->rq_prog ||
		    dr->dr_vers != req->rq_vers ||
		    dr->dr_proc != req->rq_proc ||
		    bcmp((caddr_t)&dr->dr_addr,
			(caddr_t)&req->rq_xprt->xp_raddr,
			sizeof (dr->dr_addr)) != 0) {
			dr = dr->dr_chain;
			continue;
		} else {
			dupreqs++;
			/*
			 * Should probably guard against zeroing
			 * res in case there isn't a saved response
			 * for some reason.
			 */
			if ((dr->dr_resp) && (size))
				bcopy(dr->dr_resp, res, (u_int)size);
			return (dr->dr_status);
		}
	}
	return (0);
}

/*
 * svckudp_dupdone searches the request cache and marks the
 * request done (DUP_SUCCESS or DUP_FAILED) and stores the
 * response.  If the request is not found, then the cache is
 * too small!
 */
svckudp_dupdone(req, res, size)
	register struct svc_req *req;
	register caddr_t res;
	register int	size;
{
	register struct dupreq *dr;
	u_long xid;

	xid = REQTOXID(req);
	dr = drhashtbl[XIDHASH(xid)];
	while (dr != NULL) {
		if (dr->dr_xid != xid ||
		    dr->dr_prog != req->rq_prog ||
		    dr->dr_vers != req->rq_vers ||
		    dr->dr_proc != req->rq_proc ||
		    bcmp((caddr_t)&dr->dr_addr,
			(caddr_t)&req->rq_xprt->xp_raddr,
			sizeof (dr->dr_addr)) != 0) {
			dr = dr->dr_chain;
			continue;
		} else {
			dr->dr_status = DUP_DONE;
			if ((res != (caddr_t)0) && (size != 0)) {
				/*
				 * should test if resp == NULL first
				 */
				bcopy(res, dr->dr_resp, (u_int)size);
			}
			return;
		}
	}
	/*
	 * XXX - if not found, then should warn that maybe there's not
	 * enough dup req cache space !
	 */
}

/*
 * svckudp_dupdrop searches the request cache and marks the
 * request dropped (DUP_DROP) 
 */
svckudp_dupdrop(req, res, size)
	register struct svc_req *req;
	register caddr_t res;
	register int	size;
{
	register struct dupreq *dr;
	u_long xid;

	xid = REQTOXID(req);
	dr = drhashtbl[XIDHASH(xid)];
	while (dr != NULL) {
		if (dr->dr_xid != xid ||
		    dr->dr_prog != req->rq_prog ||
		    dr->dr_vers != req->rq_vers ||
		    dr->dr_proc != req->rq_proc ||
		    bcmp((caddr_t)&dr->dr_addr,
			(caddr_t)&req->rq_xprt->xp_raddr,
			sizeof (dr->dr_addr)) != 0) {
			dr = dr->dr_chain;
			continue;
		} else {
			dr->dr_status = DUP_DROP;
			if ((res != (caddr_t)0) && (size != 0)) {
				/*
				 * should test if resp == NULL first
				 */
				bcopy(res, dr->dr_resp, (u_int)size);
			}
			return;
		}
	}
	/*
	 * XXX - if not found, then should warn that maybe there's not
	 * enough dup req cache space !
	 */
}

static
unhash(dr)
	struct dupreq *dr;
{
	struct dupreq *drt;
	struct dupreq *drtprev = NULL;

	drt = drhashtbl[DRHASH(dr)];
	while (drt != NULL) {
		if (drt == dr) {
			if (drtprev == NULL) {
				drhashtbl[DRHASH(dr)] = drt->dr_chain;
			} else {
				drtprev->dr_chain = drt->dr_chain;
			}
			return;
		}
		drtprev = drt;
		drt = drt->dr_chain;
	}
}
#endif	/* KERNEL */
