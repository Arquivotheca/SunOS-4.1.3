#if	!defined(lint) && defined(SCCSIDS)
static char sccsid[] = "@(#)svc_udp.c 1.1 92/07/30 Copyr 1984 Sun Micro";
#endif

/*
 * svc_udp.c,
 * Server side for UDP/IP based RPC.  (Does some caching in the hopes of
 * achieving execute-at-most-once semantics.)
 *
 * Copyright (C) 1984, Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <rpc/rpc.h>
#include <sys/socket.h>
#include <errno.h>
#include <sys/syslog.h>


#define	rpc_buffer(xprt) ((xprt)->xp_p1)
#define	MAX(a, b)	((a > b) ? a : b)

static struct xp_ops *svcudp_ops();

extern int errno;

/*
 * kept in xprt->xp_p2
 */
struct	svcudp_data {
	u_int   su_iosz;	/* byte size of send.recv buffer */
	u_long	su_xid;		/* transaction id */
	XDR	su_xdrs;	/* XDR handle */
	char	su_verfbody[MAX_AUTH_BYTES];	/* verifier body */
	char * 	su_cache;	/* cached data, NULL if no cache */
};
#define	su_data(xprt)	((struct svcudp_data *)(xprt->xp_p2))

/*
 * Usage:
 *	xprt = svcudp_create(sock);
 *
 * If sock<0 then a socket is created, else sock is used.
 * If the socket, sock is not bound to a port then svcudp_create
 * binds it to an arbitrary port.  In any (successful) case,
 * xprt->xp_sock is the registered socket number and xprt->xp_port is the
 * associated port number.
 * Once *xprt is initialized, it is registered as a transporter;
 * see (svc.h, xprt_register).
 * The routines returns NULL if a problem occurred.
 */
SVCXPRT *
svcudp_bufcreate(sock, sendsz, recvsz)
	register int sock;
	u_int sendsz, recvsz;
{
	bool_t madesock = FALSE;
	register SVCXPRT *xprt;
	register struct svcudp_data *su;
	struct sockaddr_in addr;
	int len = sizeof (struct sockaddr_in);

	if (sock == RPC_ANYSOCK) {
		if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
			(void) syslog(LOG_ERR,
			    "svcudp_create: socket creation problem: %m");
			return ((SVCXPRT *)NULL);
		}
		madesock = TRUE;
	}
	bzero((char *)&addr, sizeof (addr));
	addr.sin_family = AF_INET;
	if (bindresvport(sock, &addr)) {
		addr.sin_port = 0;
		(void)bind(sock, (struct sockaddr *)&addr, len);
	}
	if (getsockname(sock, (struct sockaddr *)&addr, &len) != 0) {
		(void) syslog(LOG_ERR,
		    "svcudp_create - cannot getsockname: %m");
		if (madesock)
			(void)close(sock);
		return ((SVCXPRT *)NULL);
	}
	xprt = (SVCXPRT *)mem_alloc(sizeof (SVCXPRT));
	if (xprt == NULL) {
		(void) syslog(LOG_ERR, "svcudp_create: out of memory");
		if (madesock)
			(void)close(sock);
		return ((SVCXPRT *)NULL);
	}
	su = (struct svcudp_data *)mem_alloc(sizeof (*su));
	if (su == NULL) {
		(void) syslog(LOG_ERR, "svcudp_create: out of memory");
		mem_free((char *) xprt, sizeof (SVCXPRT));
		if (madesock)
			(void)close(sock);
		return ((SVCXPRT *)NULL);
	}
	su->su_iosz = ((MAX(sendsz, recvsz) + 3) / 4) * 4;
	if ((rpc_buffer(xprt) = mem_alloc(su->su_iosz)) == NULL) {
		(void) syslog(LOG_ERR, "svcudp_create: out of memory");
		mem_free((char *) su, sizeof (*su));
		mem_free((char *) xprt, sizeof (SVCXPRT));
		if (madesock)
			(void)close(sock);
		return ((SVCXPRT *)NULL);
	}
	xdrmem_create(
	    &(su->su_xdrs), rpc_buffer(xprt), su->su_iosz, XDR_DECODE);
	su->su_cache = NULL;
	xprt->xp_p2 = (caddr_t)su;
	xprt->xp_p3 = NULL;
	xprt->xp_verf.oa_base = su->su_verfbody;
	xprt->xp_ops = svcudp_ops();
	xprt->xp_port = ntohs(addr.sin_port);
	xprt->xp_sock = sock;
	xprt_register(xprt);
	return (xprt);
}

SVCXPRT *
svcudp_create(sock)
	int sock;
{

	return (svcudp_bufcreate(sock, UDPMSGSIZE, UDPMSGSIZE));
}

static enum xprt_stat
svcudp_stat(xprt)
	SVCXPRT *xprt;
{

	return (XPRT_IDLE);
}

static bool_t
svcudp_recv(xprt, msg)
	register SVCXPRT *xprt;
	struct rpc_msg *msg;
{
	register struct svcudp_data *su = su_data(xprt);
	register XDR *xdrs = &(su->su_xdrs);
	register int rlen;
	char *reply;
	u_long replylen;

again:
	xprt->xp_addrlen = sizeof (struct sockaddr_in);
	rlen = recvfrom(xprt->xp_sock, rpc_buffer(xprt), (int) su->su_iosz,
	    0, (struct sockaddr *)&(xprt->xp_raddr), &(xprt->xp_addrlen));
	if (rlen == -1 && errno == EINTR)
		goto again;
	if (rlen < 4*sizeof (u_long))
		return (FALSE);
	xdrs->x_op = XDR_DECODE;
	XDR_SETPOS(xdrs, 0);
	if (! xdr_callmsg(xdrs, msg))
		return (FALSE);
	su->su_xid = msg->rm_xid;
	if (su->su_cache != NULL) {
		if (cache_get(xprt, msg, &reply, &replylen)) {
			(void) sendto(xprt->xp_sock, reply, (int) replylen, 0,
			    (struct sockaddr *) &xprt->xp_raddr,
			    xprt->xp_addrlen);
			return (FALSE);
		}
	}
	return (TRUE);
}

static bool_t
svcudp_reply(xprt, msg)
	register SVCXPRT *xprt;
	struct rpc_msg *msg;
{
	register struct svcudp_data *su = su_data(xprt);
	register XDR *xdrs = &(su->su_xdrs);
	register int slen;
	register bool_t stat = FALSE;

	xdrs->x_op = XDR_ENCODE;
	XDR_SETPOS(xdrs, 0);
	msg->rm_xid = su->su_xid;
	if (xdr_replymsg(xdrs, msg)) {
		slen = (int)XDR_GETPOS(xdrs);
		if (sendto(xprt->xp_sock, rpc_buffer(xprt), slen, 0,
		    (struct sockaddr *)&(xprt->xp_raddr), xprt->xp_addrlen)
		    == slen) {
			stat = TRUE;
			if (su->su_cache && slen >= 0) {
				cache_set(xprt, (u_long) slen);
			}
		}
	}
	return (stat);
}

static bool_t
svcudp_getargs(xprt, xdr_args, args_ptr)
	SVCXPRT *xprt;
	xdrproc_t xdr_args;
	caddr_t args_ptr;
{

	return ((*xdr_args)(&(su_data(xprt)->su_xdrs), args_ptr));
}

static bool_t
svcudp_freeargs(xprt, xdr_args, args_ptr)
	SVCXPRT *xprt;
	xdrproc_t xdr_args;
	caddr_t args_ptr;
{
	register XDR *xdrs = &(su_data(xprt)->su_xdrs);

	xdrs->x_op = XDR_FREE;
	return ((*xdr_args)(xdrs, args_ptr));
}

static void
svcudp_destroy(xprt)
	register SVCXPRT *xprt;
{
	register struct svcudp_data *su = su_data(xprt);

	xprt_unregister(xprt);
	(void)close(xprt->xp_sock);
	XDR_DESTROY(&(su->su_xdrs));
	mem_free(rpc_buffer(xprt), su->su_iosz);
	mem_free((caddr_t)su, sizeof (struct svcudp_data));
	mem_free((caddr_t)xprt, sizeof (SVCXPRT));
}


/***********this could be a separate file*********************/

/*
 * Fifo cache for udp server
 * Copies pointers to reply buffers into fifo cache
 * Buffers are sent again if retransmissions are detected.
 */

#define	SPARSENESS 4	/* 75% sparse */

#define	ALLOC(type, size)	\
	(type *) mem_alloc((unsigned) (sizeof (type) * (size)))

#define	BZERO(addr, type, size)	 \
	bzero((char *) (addr), sizeof (type) * (int) (size))

#define	FREE(addr, type, size)	\
	mem_free((char *) (addr), (sizeof (type) * (size)))

/*
 * An entry in the cache
 */
typedef	struct cache_node *cache_ptr;
struct	cache_node {
	/*
	 * Index into cache is xid, proc, vers, prog and address
	 */
	u_long cache_xid;
	u_long cache_proc;
	u_long cache_vers;
	u_long cache_prog;
	struct sockaddr_in cache_addr;
	/*
	 * The cached reply and length
	 */
	char * cache_reply;
	u_long cache_replylen;
	/*
	 * Next node on the list, if there is a collision
	 */
	cache_ptr cache_next;
};



/*
 * The entire cache
 */
struct	udp_cache {
	u_long uc_size;		/* size of cache */
	cache_ptr *uc_entries;	/* hash table of entries in cache */
	cache_ptr *uc_fifo;	/* fifo list of entries in cache */
	u_long uc_nextvictim;	/* points to next victim in fifo list */
	u_long uc_prog;		/* saved program number */
	u_long uc_vers;		/* saved version number */
	u_long uc_proc;		/* saved procedure number */
	struct sockaddr_in uc_addr; /* saved caller's address */
};


/*
 * the hashing function
 */
#define	CACHE_LOC(transp, xid)	\
(xid % (SPARSENESS*((struct udp_cache *) su_data(transp)->su_cache)->uc_size))


/*
 * Enable use of the cache.
 * Note: there is no disable.
 */
svcudp_enablecache(transp, size)
	SVCXPRT *transp;
	u_long size;
{
	struct svcudp_data *su = su_data(transp);
	struct udp_cache *uc;

	if (su->su_cache != NULL) {
		(void) syslog(LOG_ERR, "enablecache: cache already enabled");
		return (0);
	}
	uc = ALLOC(struct udp_cache, 1);
	if (uc == NULL) {
		(void) syslog(LOG_ERR, "enablecache: could not allocate cache");
		return (0);
	}
	uc->uc_size = size;
	uc->uc_nextvictim = 0;
	uc->uc_entries = ALLOC(cache_ptr, size * SPARSENESS);
	if (uc->uc_entries == NULL) {
		(void) syslog(LOG_ERR,
		    "enablecache: could not allocate cache data");
		FREE(uc, struct udp_cache, 1);
		return (0);
	}
	BZERO(uc->uc_entries, cache_ptr, size * SPARSENESS);
	uc->uc_fifo = ALLOC(cache_ptr, size);
	if (uc->uc_fifo == NULL) {
		(void) syslog(LOG_ERR,
		    "enablecache: could not allocate cache fifo");
		FREE(uc->uc_entries, cache_ptr, size * SPARSENESS);
		FREE(uc, struct udp_cache, 1);
		return (0);
	}
	BZERO(uc->uc_fifo, cache_ptr, size);
	su->su_cache = (char *) uc;
	return (1);
}


/*
 * Set an entry in the cache
 */
static
cache_set(xprt, replylen)
	SVCXPRT *xprt;
	u_long replylen;
{
	register cache_ptr victim;
	register cache_ptr *vicp;
	register struct svcudp_data *su = su_data(xprt);
	struct udp_cache *uc = (struct udp_cache *) su->su_cache;
	u_int loc;
	char *newbuf;

	/*
	 * Find space for the new entry, either by
	 * reusing an old entry, or by mallocing a new one
	 */
	victim = uc->uc_fifo[uc->uc_nextvictim];
	if (victim != NULL) {
		loc = CACHE_LOC(xprt, victim->cache_xid);
		for (vicp = &uc->uc_entries[loc];
		    *vicp != NULL && *vicp != victim;
		    vicp = &(*vicp)->cache_next);

		if (*vicp == NULL) {
			(void) syslog(LOG_ERR, "cache_set: victim not found");
			return;
		}
		*vicp = victim->cache_next;	/* remote from cache */
		newbuf = victim->cache_reply;
	} else {
		victim = ALLOC(struct cache_node, 1);
		if (victim == NULL) {
			(void) syslog(LOG_ERR,
			    "cache_set: victim alloc failed");
			return;
		}
		newbuf = mem_alloc(su->su_iosz);
		if (newbuf == NULL) {
			(void) syslog(LOG_ERR,
			    "cache_set: could not allocate new rpc_buffer");
			FREE(victim, struct cache_node, 1);
			return;
		}
	}

	/*
	 * Store it away
	 */
	victim->cache_replylen = replylen;
	victim->cache_reply = rpc_buffer(xprt);
	rpc_buffer(xprt) = newbuf;
	xdrmem_create(&(su->su_xdrs),
	    rpc_buffer(xprt), su->su_iosz, XDR_ENCODE);
	victim->cache_xid = su->su_xid;
	victim->cache_proc = uc->uc_proc;
	victim->cache_vers = uc->uc_vers;
	victim->cache_prog = uc->uc_prog;
	victim->cache_addr = uc->uc_addr;
	loc = CACHE_LOC(xprt, victim->cache_xid);
	victim->cache_next = uc->uc_entries[loc];
	uc->uc_entries[loc] = victim;
	uc->uc_fifo[uc->uc_nextvictim++] = victim;
	uc->uc_nextvictim %= uc->uc_size;
}

/*
 * Try to get an entry from the cache
 * return 1 if found, 0 if not found
 */
static
cache_get(xprt, msg, replyp, replylenp)
	SVCXPRT *xprt;
	struct rpc_msg *msg;
	char **replyp;
	u_long *replylenp;
{
	u_int loc;
	register cache_ptr ent;
	register struct svcudp_data *su = su_data(xprt);
	register struct udp_cache *uc = (struct udp_cache *) su->su_cache;

#	define EQADDR(a1, a2)	(bcmp((char*)&a1, (char*)&a2, sizeof (a1)) == 0)

	loc = CACHE_LOC(xprt, su->su_xid);
	for (ent = uc->uc_entries[loc]; ent != NULL; ent = ent->cache_next) {
		if (ent->cache_xid == su->su_xid &&
		    ent->cache_proc == uc->uc_proc &&
		    ent->cache_vers == uc->uc_vers &&
		    ent->cache_prog == uc->uc_prog &&
		    EQADDR(ent->cache_addr, uc->uc_addr)) {
			*replyp = ent->cache_reply;
			*replylenp = ent->cache_replylen;
			return (1);
		}
	}
	/*
	 * Failed to find entry
	 * Remember a few things so we can do a set later
	 */
	uc->uc_proc = msg->rm_call.cb_proc;
	uc->uc_vers = msg->rm_call.cb_vers;
	uc->uc_prog = msg->rm_call.cb_prog;
	uc->uc_addr = xprt->xp_raddr;
	return (0);
}

static struct xp_ops *
svcudp_ops()
{
	static struct xp_ops ops;

	if (ops.xp_recv == NULL) {
		ops.xp_recv = svcudp_recv;
		ops.xp_stat = svcudp_stat;
		ops.xp_getargs = svcudp_getargs;
		ops.xp_reply = svcudp_reply;
		ops.xp_freeargs = svcudp_freeargs;
		ops.xp_destroy = svcudp_destroy;
	}
	return (&ops);
}
