#ifndef lint
static char sccsid[] = "@(#)clnt_kudp.c 1.1 92/07/30 Copyr 1989 Sun Micro";
#endif

/*
 * clnt_kudp.c
 * Implements a kernel UPD/IP based, client side RPC.
 *
 * Copyright (C) 1984, Sun Microsystems, Inc.
 */

#include <sys/param.h>
#include "boot/systm.h"
#include <sys/user.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/mbuf.h>
#include <net/if.h>
#include <net/route.h>
#include <netinet/in.h>
#include <netinet/in_pcb.h>
#include <rpc/types.h>
#include <rpc/xdr.h>
#include <rpc/auth.h>
#include <rpc/clnt.h>
#include <rpc/rpc_msg.h>
#include <mon/sunromvec.h>

extern	int	verbosemode;

#ifdef	NFS_BOOT
#undef	u
extern struct user u;
#endif	/* NFS_BOOT */

#ifdef OPENPROMS
#define	millitime()	prom_gettime()
#else
#define	millitime()	(*romp->v_nmiclock)
#endif !OPENPROMS

struct mbuf	*ku_recvfrom();
int		ckuwakeup();

enum clnt_stat	clntkudp_callit();
void		clntkudp_abort();
void		clntkudp_error();
bool_t		clntkudp_freeres();
void		clntkudp_destroy();

void		xdrmbuf_init();

/*
 * Operations vector for UDP/IP based RPC
 */
static struct clnt_ops udp_ops = {
	clntkudp_callit,	/* do rpc call */
	clntkudp_abort,		/* abort call */
	clntkudp_error,		/* return error status */
	clntkudp_freeres,	/* free results */
	clntkudp_destroy	/* destroy rpc handle */
};

/*
 * Private data per rpc handle.  This structure is allocated by
 * clntkudp_create, and freed by cku_destroy.
 */
struct cku_private {
	u_int			 cku_flags;	/* see below */
	CLIENT			 cku_client;	/* client handle */
	int			 cku_retrys;	/* request retrys */
	struct socket		*cku_sock;	/* open udp socket */
	struct sockaddr_in	 cku_addr;	/* remote address */
	struct rpc_err		 cku_err;	/* error status */
	XDR			 cku_outxdr;	/* xdr routine for output */
	XDR			 cku_inxdr;	/* xdr routine for input */
	u_int			 cku_outpos;	/* position of in output mbuf */
	char			*cku_outbuf;	/* output buffer */
	char			*cku_inbuf;	/* input buffer */
	struct mbuf		*cku_inmbuf;	/* input mbuf */
	struct ucred		*cku_cred;	/* credentials */
};

struct {
	int	rccalls;
	int	rcbadcalls;
	int	rcretrans;
	int	rcbadxids;
	int	rctimeouts;
	int	rcwaits;
	int	rcnewcreds;
} rcstat;


#define	ptoh(p)		(&((p)->cku_client))
#define	htop(h)		((struct cku_private *)((h)->cl_private))

/* cku_flags */
#define	CKU_TIMEDOUT	0x001
#define	CKU_BUSY	0x002
#define	CKU_WANTED	0x004
#define	CKU_BUFBUSY	0x008
#define	CKU_BUFWANTED	0x010

#define	RECVTRIES	18	/* # retransmission attempts */
#define	MAXRTO		16000	/* max retransmission timer in ms. */
#define	ONESECOND	1000	/* 1 second == 1000 ms */

int	clntkudpxid = 0;

/*
 * V. Jacobson Mean+Varience retransmission timer algorithm.
 * Best guess starting values.
 */
static	int	sa = 800;	/* scaled average round trip time */
static	int	sd = 20;	/* scaled mean deviation */
static	int	rto = 1000;	/* retransmit timer */


static
noop()
{
}

static
buffree(p)
	struct cku_private *p;
{
#ifdef	RPCDEBUG
	rpc_debug(6, "buffree(p 0x%x)\n", p);
#endif	/* RPCDEBUG */
	p->cku_flags &= ~CKU_BUFBUSY;
	if (p->cku_flags & CKU_BUFWANTED) {
		p->cku_flags &= ~CKU_BUFWANTED;

		wakeup((caddr_t)&p->cku_outbuf);
	}
}

/*
 * Create an rpc handle for a udp rpc connection.
 * Allocates space for the handle structure and the private data, and
 * opens a socket.  Note sockets and handles are one to one.
 */
CLIENT *
clntkudp_create(addr, pgm, vers, retrys, cred)
	struct sockaddr_in *addr;
	u_long pgm;
	u_long vers;
	int retrys;
	struct ucred *cred;
{
	register CLIENT *h;
	register struct cku_private *p;
	int error = 0;
	struct rpc_msg call_msg;
	struct mbuf *m, *mclgetx();
	extern int nfs_portmon;

#ifdef RPCDEBUG
	rpc_debug(6, "clntkudp_create(addr 0x%x pgm %d vers %d retrys %d\n",
	    addr->sin_addr.s_addr, pgm, vers, retrys);
#endif
	p = (struct cku_private *)kmem_alloc((u_int)sizeof *p);
	bzero((caddr_t)p, sizeof (*p));
	h = ptoh(p);

	if (!clntkudpxid) {
		clntkudpxid = 0x1f7391f3 + millitime();
	}

	/* handle */
	h->cl_ops = &udp_ops;
	h->cl_private = (caddr_t) p;
	h->cl_auth = authkern_create();

	/* call message, just used to pre-serialize below */
	call_msg.rm_xid = 0;
	call_msg.rm_direction = CALL;
	call_msg.rm_call.cb_rpcvers = RPC_MSG_VERSION;
	call_msg.rm_call.cb_prog = pgm;
	call_msg.rm_call.cb_vers = vers;

	/* private */
	clntkudp_init(h, addr, retrys, cred);
	p->cku_outbuf = (char *)kmem_alloc((u_int)UDPMSGSIZE);
	m = mclgetx(noop, 0, p->cku_outbuf, UDPMSGSIZE, M_DONTWAIT);
	if (m == NULL)
		goto bad;
	xdrmbuf_init(&p->cku_outxdr, m, XDR_ENCODE);

	/* pre-serialize call message header */
	if (! xdr_callhdr(&(p->cku_outxdr), &call_msg)) {
		printf("clntkudp_create - Fatal header serialization error.");
		(void) m_freem(m);
		goto bad;
	}
	p->cku_outpos = XDR_GETPOS(&(p->cku_outxdr));
	(void) m_free(m);

	/* open udp socket */
	error = socreate(AF_INET, &p->cku_sock, SOCK_DGRAM, IPPROTO_UDP);
	if (error) {
		printf("clntkudp_create: socket creation problem: ");
		errno_print(error);
		printf("\n");
		goto bad;
	}

	if (error = bindresvport(p->cku_sock)) {
		printf("clntkudp_create: socket bind problem: ");
		errno_print(error);
		printf("\n");
		goto bad;
	}
	return (h);

bad:
	kmem_free((caddr_t)p->cku_outbuf, (u_int)UDPMSGSIZE);
	kmem_free((caddr_t)p, (u_int)sizeof (struct cku_private));
#ifdef RPCDEBUG
	rpc_debug(10, "clntkudp_create: create failed\n");
#endif
	return ((CLIENT *)NULL);
}

clntkudp_init(h, addr, retrys, cred)
	CLIENT *h;
	struct sockaddr_in *addr;
	int retrys;
	struct ucred *cred;
{
	struct cku_private *p = htop(h);

#ifdef RPCDEBUG
	rpc_debug(6, "clntkudp_init(h 0x%x addr 0x%x retrys 0x%x)\n",
		h, addr, retrys);
#endif

	p->cku_retrys = retrys;
	p->cku_addr = *addr;
	p->cku_cred = cred;
	p->cku_flags = 0;
}

/*
 * Time out back off function. tim is in hz
 */
#define	MAXTIMO	(60 * hz)
#define	backoff(tim)	((((tim) << 1) > MAXTIMO) ? MAXTIMO : ((tim) << 1))

/*
 * Call remote procedure.
 * Most of the work of rpc is done here.  We serialize what is left
 * of the header (some was pre-serialized in the handle), serialize
 * the arguments, and send it off.  We wait for a reply or a time out.
 * Timeout causes an immediate return, other packet problems may cause
 * a retry on the receive.  When a good packet is received we deserialize
 * it, and check verification.  A bad reply code will cause one retry
 * with full (longhand) credentials.
 * If "ignorebad" is TRUE, bad (!RPC_SUCCESS) replies are ignored.
 */
enum clnt_stat
clntkudp_callit_addr(h, procnum, xdr_args, argsp, xdr_results, resultsp, wait,
	    sin, ignorebad)
	register CLIENT	*h;
	u_long		procnum;
	xdrproc_t	xdr_args;
	caddr_t		argsp;
	xdrproc_t	xdr_results;
	caddr_t		resultsp;
	struct timeval	wait;
	struct sockaddr_in *sin;
	int		ignorebad;
{
	register struct cku_private *p = htop(h);
	register XDR		   *xdrs;
	register struct socket	   *so = p->cku_sock;
	int			   rtries = 0;
	struct sockaddr_in	   from;
	struct rpc_msg		   reply_msg;
	int			   s;
	struct ucred		   *tmpcred;
	struct mbuf		   *m;
	u_long xid;
	int	t1, t2, tout;
	int	maxtout;

#ifdef RPCDEBUG
	rpc_debug(6,
	    "clntkudp_callit_addr(h 0x%x procnum 0x%x xdr_args 0x%x argsp 0x%x\n",
	    h, procnum, xdr_args, argsp);
	rpc_debug(6,
		"xdr_results 0x%x resultsp 0x%x wait 0x%x sin 0x%x)\n",
		xdr_results, resultsp, wait, sin);
#endif
	rcstat.rccalls++;

	/*
	 * compute absolute return time = <now> + <wait argument> in ms
	 */
	maxtout = millitime() + (wait.tv_sec * 1000) + (wait.tv_usec / 1000);

	while (p->cku_flags & CKU_BUSY) {
		rcstat.rcwaits++;
		p->cku_flags |= CKU_WANTED;
		(void) sleep((caddr_t)h, PZERO-2);
	}
	p->cku_flags |= CKU_BUSY;

	/*
	 * Set credentials into the u structure
	 */
	tmpcred = u.u_cred;
	u.u_cred = p->cku_cred;
	xid = clntkudpxid++;

call_again:

	/*
	 * Wait til buffer gets freed then make a type 2 mbuf point at it
	 * The buffree routine clears CKU_BUFBUSY and does a wakeup when
	 * the mbuf gets freed.
	 */
	s = splimp();
	while (p->cku_flags & CKU_BUFBUSY) {
		p->cku_flags |= CKU_BUFWANTED;
		/*
		 * This is a kludge to avoid deadlock in the case of a
		 * loop-back call.  The client can block waiting for
		 * the server to free the mbuf while the server is blocked
		 * waiting for the client to free the reply mbuf.  Avoid this
		 * by flushing the input queue every once in a while while
		 * we are waiting.
		 */
		timeout(wakeup, (caddr_t)&p->cku_outbuf, hz);
	}
	p->cku_flags |= CKU_BUFBUSY;
	(void) splx(s);
	m = mclgetx(buffree, (int)p, p->cku_outbuf, UDPMSGSIZE, M_WAIT);
	if (m == NULL) {
		p->cku_err.re_status = RPC_SYSTEMERROR;
		p->cku_err.re_errno = ENOBUFS;
		buffree(p);
		goto done;
	}

	/*
	 * The transaction id is the first thing in the
	 * preserialized output buffer.
	 */
	(*(u_long *)(p->cku_outbuf)) = xid;

	xdrmbuf_init(&p->cku_outxdr, m, XDR_ENCODE);
	xdrs = &p->cku_outxdr;
	XDR_SETPOS(xdrs, p->cku_outpos);

	/*
	 * Serialize dynamic stuff into the output buffer.
	 */
	if ((! XDR_PUTLONG(xdrs, (long *)&procnum)) ||
	    (! AUTH_MARSHALL(h->cl_auth, xdrs)) ||
	    (! (*xdr_args)(xdrs, argsp))) {
		p->cku_err.re_status = RPC_CANTENCODEARGS;
		(void) m_freem(m);
		goto done;
	}

	if (m->m_next == 0) {		/* XXX */
		m->m_len = XDR_GETPOS(&(p->cku_outxdr));
	}
	if (p->cku_err.re_errno =
	    ku_sendto_mbuf(so, m, &p->cku_addr)) {
		p->cku_err.re_status = RPC_CANTSEND;
		goto done;
	}

	/*
	 * Start the roundtrip timer and calculate retransmit time.
	 */
	t1 = millitime();
	tout = t1 + rto;

	reply_msg.acpted_rply.ar_verf = _null_auth;
	reply_msg.acpted_rply.ar_results.where = resultsp;
	reply_msg.acpted_rply.ar_results.proc = xdr_results;


recv_again:
	while (rtries < RECVTRIES) {
		s = splnet();
		while (so->so_rcv.sb_cc == 0) {
			if (get_udp() != 0)
				continue;
			if ((t2 = millitime()) > tout) {
				(void) splx(s);
				p->cku_err.re_status = RPC_TIMEDOUT;
				p->cku_err.re_errno = ETIMEDOUT;
				rcstat.rctimeouts++;
				rcstat.rcretrans++;
				rtries++;

				if (t2 > maxtout)	/* time to return */
					goto done;

				/*
				 * Update average and mean deviation and keep
				 * doubling the retransmission timeout until we
				 * get a good return.
				 */
				rtupdate(t2 - t1);
				rto = rto << 1;		/* double rto */

				/*
				 * After 2 retransmissions, bump the
				 * retransmission timeout to at least 1 second.
				 * This is the el-cheapo method of flood
				 * avoidance. The Real method would be to keep
				 * sa, sd, and rto for each of several classes
				 * of rpc since mount requests take an order of
				 * magnitude or longer than read requests but,
				 * hey, this is just the boot program and we're
				 * lucky to get adaptive retransmits at all.
				 */
				if (rtries > 1)
					rto = (rto < ONESECOND)? ONESECOND: rto;
				if (rto > MAXRTO)
					rto = MAXRTO;

				buffree(p);

				if (verbosemode)
					printf("X");
				goto call_again;
			}

		}

		/*
		 * Update sa, sd, and rto.
		 */
		t2 = millitime();
		rto = rtupdate(t2 - t1);

		if (so->so_error) {
			so->so_error = 0;
			(void) splx(s);
			continue;
		}


		p->cku_inmbuf = ku_recvfrom(so, &from);
		if (sin) {
			*sin = from;
		}
#ifdef	RPCDEBUG
		rpc_debug(6, "clntkudp_callit_addr: cku_inmbuf 0x%x\n",
			p->cku_inmbuf);
#endif	/* RPCDEBUG */

		(void) splx(s);
		if (p->cku_inmbuf == NULL) {
			continue;
		}
		p->cku_inbuf = mtod(p->cku_inmbuf, char *);

		if (p->cku_inmbuf->m_len < sizeof (u_long)) {
			m_freem(p->cku_inmbuf);
			continue;
		}
		/*
		 * If reply transaction id matches id sent
		 * we have a good packet.
		 */
		if (*((u_long *)(p->cku_inbuf))
		    != *((u_long *)(p->cku_outbuf))) {
#ifdef	RPCDEBUG
		rpc_debug(6, "clntkudp_callit_addr: bad xid 0x%x not 0x%x\n",
		    *((u_long *)(p->cku_inbuf)), *((u_long *)(p->cku_outbuf)));
#endif	/* RPCDEBUG */
			rcstat.rcbadxids++;
			m_freem(p->cku_inmbuf);
			continue;
		}

		break;

	}

	if (rtries > RECVTRIES) {
#ifdef	RPCDEBUG
	rpc_debug(1, "clntkudp_callit retries exhausted\n");
#endif	/* RPCDEBUG */
		if (verbosemode)
			printf("retries exhausted, giving up\n");
		p->cku_err.re_status = RPC_CANTRECV;
		goto done;
	}

	/*
	 * Process reply
	 */

	xdrs = &(p->cku_inxdr);
	xdrmbuf_init(xdrs, p->cku_inmbuf, XDR_DECODE);


	/*
	 * Decode and validate the response.
	 */
	if (xdr_replymsg(xdrs, &reply_msg)) {
		_seterr_reply(&reply_msg, &(p->cku_err));

		if (p->cku_err.re_status == RPC_SUCCESS) {
			/*
			 * Reply is good, check auth.
			 */
			if (! AUTH_VALIDATE(h->cl_auth,
			    &reply_msg.acpted_rply.ar_verf)) {
				if (verbosemode) {
					clntstat_print(RPC_AUTHERROR);
					printf("\n");
				}
			    rpc_debug(3, "clntkudp_callit_addr: bad auth\n");
				p->cku_err.re_status = RPC_AUTHERROR;
				p->cku_err.re_why = AUTH_INVALIDRESP;
			}

		}
	} else {
		if (verbosemode) {
			clntstat_print(RPC_CANTDECODERES);
			printf("\n");
		}
		rpc_debug(10, "clntkudp_callit_addr: RPC_CANTDECODERES\n");
		p->cku_err.re_status = RPC_CANTDECODERES;
	}
	m_freem(p->cku_inmbuf);


#ifdef RPCDEBUG
	rpc_debug(6, "clntkudp_callit_addr done\n");
#endif
done:

	if (verbosemode && (p->cku_err.re_status != RPC_SUCCESS)) {
		printf("call ");
		clntstat_print(p->cku_err.re_status);
		printf("\n");
	}


	/*
	 * Weed out remote rpc error replies.
	 */
	if (ignorebad &&
		((p->cku_err.re_status == RPC_VERSMISMATCH) ||
		    (p->cku_err.re_status == RPC_AUTHERROR) ||
		    (p->cku_err.re_status == RPC_PROGUNAVAIL) ||
		    (p->cku_err.re_status == RPC_PROGVERSMISMATCH) ||
		    (p->cku_err.re_status == RPC_PROCUNAVAIL) ||
		    (p->cku_err.re_status == RPC_CANTDECODEARGS) ||
		    (p->cku_err.re_status == RPC_SYSTEMERROR))) {
		if (verbosemode)
			printf("-- ignored\n");
		goto recv_again;
	}

	u.u_cred = tmpcred;
	p->cku_flags &= ~CKU_BUSY;
	if (p->cku_flags & CKU_WANTED) {
		p->cku_flags &= ~CKU_WANTED;
#ifdef	RPCDEBUG
		rpc_debug(5, "clntkudp_callit_addr: wakeup\n");
#endif	/* RPCDEBUG */
		wakeup((caddr_t)h);
	}
	if (p->cku_err.re_status != RPC_SUCCESS) {
		rcstat.rcbadcalls++;
	}
	buffree(p);
	return (p->cku_err.re_status);
}

/*
 * V. Jacobson Mean+Varience retransmission timer alrorithm.
 * Update scaled average and mean deviation
 * and return new retransmit timer value.
 */
static
rtupdate(m)
int	m;
{
	m -= (sa >> 3);
	sa += m;
	if (m < 0)
		m = -m;
	m -= (sd >> 2);
	sd += m;
	return (((sa >> 2) + sd) >> 1);
}

enum clnt_stat
clntkudp_callit(h, procnum, xdr_args, argsp, xdr_results, resultsp, wait)
	register CLIENT	*h;
	u_long		procnum;
	xdrproc_t	xdr_args;
	caddr_t		argsp;
	xdrproc_t	xdr_results;
	caddr_t		resultsp;
	struct timeval	wait;
{
#ifdef RPCDEBUG
	rpc_debug(6,
		"clntkudp_callit(h 0x%x, procnum 0x%x, xdr_args 0x%x, argsp 0x%x, xdr_results 0x%x, resultsp 0x%x, wait 0x%x)\n",
		h, procnum, xdr_args, argsp, xdr_results, resultsp, wait);
#endif
	return (clntkudp_callit_addr(h, procnum, xdr_args, argsp, xdr_results,
		resultsp, wait, (struct sockaddr_in *)0, 0));
}

/*
 * Wake up client waiting for a reply.
 */
ckuwakeup(p)
	register struct cku_private *p;
{

#ifdef RPCDEBUG
	rpc_debug(6, "ckuwakeup\n");
#endif
	p->cku_flags |= CKU_TIMEDOUT;
}

/*
 * Return error info on this handle.
 */
void
clntkudp_error(h, err)
	CLIENT *h;
	struct rpc_err *err;
{
	register struct cku_private *p = htop(h);

	*err = p->cku_err;
}

static bool_t
clntkudp_freeres(cl, xdr_res, res_ptr)
	CLIENT *cl;
	xdrproc_t xdr_res;
	caddr_t res_ptr;
{
	register struct cku_private *p = (struct cku_private *)cl->cl_private;
	register XDR *xdrs = &(p->cku_outxdr);

	xdrs->x_op = XDR_FREE;
	return ((*xdr_res)(xdrs, res_ptr));
}

void
clntkudp_abort()
{
}

/*
 * Destroy rpc handle.
 * Frees the space used for output buffer, private data, and handle
 * structure, and closes the socket for this handle.
 */
void
clntkudp_destroy(h)
	CLIENT *h;
{

#ifdef RPCDEBUG
	rpc_debug(6, "cku_destroy %x\n", h);
#endif
}

/*
 * try to bind to a reserved port
 */
bindresvport(so)
	struct socket *so;
{
	struct sockaddr_in *sin;
	struct mbuf *m;
	u_short i;
	int error;
	int saveuid;

#define	MAX_PRIV	(IPPORT_RESERVED-1)
#define	MIN_PRIV	(IPPORT_RESERVED/2)

	m = m_get(M_WAIT, MT_SONAME);
	if (m == NULL) {
		printf("bindresvport: couldn't alloc mbuf");
		return (ENOBUFS);
	}

	sin = mtod(m, struct sockaddr_in *);
	sin->sin_family = AF_INET;
	sin->sin_addr.s_addr = INADDR_ANY;
	m->m_len = sizeof (struct sockaddr_in);

	saveuid = u.u_uid;
	u.u_uid = 0;
	error = EADDRINUSE;
	for (i = MAX_PRIV; error == EADDRINUSE && i >= MIN_PRIV; i--) {
		sin->sin_port = htons(i);
		error = sobind(so, m);
	}
	u.u_uid = saveuid;

	(void) m_freem(m);
	return (error);
}

clnt_delay(i)
	int	i;
{
	for (; i > 0; i--);
}

rpcstats()
{
	if (rcstat.rccalls > 0)
		printf("rpc:  calls %d badcalls %d retrans %d badxids %d timeouts %d waits %d newcreds %d\n",
			rcstat.rccalls,
			rcstat.rcbadcalls,
			rcstat.rcretrans,
			rcstat.rcbadxids,
			rcstat.rctimeouts,
			rcstat.rcwaits,
			rcstat.rcnewcreds);
}
