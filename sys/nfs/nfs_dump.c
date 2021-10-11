/*	@(#)nfs_dump.c 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * Dump memory to NFS swap file after a panic.
 * We can't depend on malloc, sleep, etc. after
 * a panic so we have to do everything ourselves.
 * The dumplication of functionality that exists
 * elsewhere in the NFS code is unfortunate.
 */

#include <rpc/types.h>
#include <sys/param.h>
#include <sys/errno.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/mbuf.h>
#include <sys/vnode.h>
#include <sys/vfs.h>
#include <sys/bootconf.h>
#undef NFSSERVER
#include <nfs/nfs.h>
#include <rpc/auth.h>
#include <rpc/xdr.h>
#include <rpc/rpc_msg.h>
#include <rpc/clnt.h>
#include <netinet/in.h>
#include <nfs/nfs_clnt.h>
#include <nfs/rnode.h>

#define	TIMEO		2	/* seconds before retransmitting request */
#define	MAXTRIES	5	/* before giving up on server */


/*
 * The following are filled in by config_nfsswap at boot time.
 */
struct sockaddr_in nfsdump_sin;
fhandle_t nfsdump_fhandle;
int nfsdump_maxcount;

#define	HDR_SIZE	256

static
noop()
{
}

nfs_dump(dumpvp, addr, bn, count)
	struct vnode *dumpvp;
	caddr_t addr;
	int bn;
	int count;
{
	int tsize, offset, error;
	XDR xdrs;
	static struct rpc_msg call_msg, reply_msg;
	static struct sockaddr_in from;
	static struct rpc_err rpc_err;
	int s, timeout_count, timeout_sec;
	struct mbuf *m;
	int procnum;
	static struct nfsattrstat na;
	static struct socket *dump_socket;
	static char header[HDR_SIZE];
	static int state;
	static char state_char[4] = {'-', '\\', '|', '/'};
	extern struct mbuf *ku_recvfrom();

#ifdef sun
	/*
	 * This is code is to prevent the message buffer
	 * from filling up with the nfs dumping printfs.
	 * Non-sun machines will have to use some other
	 * method to get this same effect (if it is desired).
	 */
	{
		int msgbufinit_sav;
		extern int msgbufinit;

		msgbufinit_sav = msgbufinit;
		msgbufinit = 0;
		printf("%c\b", state_char[state]);
		msgbufinit = msgbufinit_sav;
	}
#else
	printf("%c\b", state_char[state]);
#endif
	state++;
	if (state >= 4) {
		state = 0;
	}
	if (dump_socket == NULL) {
		error = socreate(AF_INET,
			(struct socket **) &dump_socket,
			SOCK_DGRAM, IPPROTO_UDP);
		if (error) {
			printf("nfsdump: socreate returned error %d\n", error);
			panic("nfsdump: cannot create socket\n");
		}
		if (error = bindresvport(dump_socket)) {
			printf(
			    "nfsdump: error %d from bindreservport\n", error);
			panic("nfsdump: cannot bind");
		}
	}
	offset = dbtob(bn);
	count <<= DEV_BSHIFT;
	/*
	 * Set up call header outside loop.
	 */
	call_msg.rm_direction = CALL;
	call_msg.rm_call.cb_rpcvers = RPC_MSG_VERSION;
	call_msg.rm_call.cb_prog = NFS_PROGRAM;
	call_msg.rm_call.cb_vers = NFS_VERSION;
	timeout_count = 0;
	do {
resend:
		timeout_sec = time.tv_sec + (timeout_count + 1) * TIMEO;
		/*
		 * Mbuf to contain call header.
		 */
		m = (struct mbuf *)mclgetx(noop, 0, header, HDR_SIZE, M_WAIT);
		if (m == NULL) {
			printf("nfsdump: out of mbufs\n");
			return (ENOBUFS);
		}
		xdrmbuf_init(&xdrs, m, XDR_ENCODE);
		call_msg.rm_xid = alloc_xid();
		if (! xdr_callhdr(&xdrs, &call_msg)) {
			printf("nfsdump: cannot serialize header\n");
			return (EIO);
		}
		procnum = RFS_WRITE;
		tsize = MIN(vtomi(dumpvp)->mi_stsize, count);
		if (nfsdump_maxcount) {
			/*
			 * Do not extend the dump file if it is also
			 * the swap file.
			 */
			if (offset >= nfsdump_maxcount) {
				printf("nfsdump: end of file\n");
				return (EIO);
			}
			if (offset + tsize > nfsdump_maxcount) {
				tsize = nfsdump_maxcount - offset;
			}
		}
		if (!XDR_PUTLONG(&xdrs, (long *)&procnum) ||
		    !dump_auth_marshall(&xdrs) ||
		    !xdr_fhandle(&xdrs, &nfsdump_fhandle) ||
		    !XDR_PUTLONG(&xdrs, &offset) ||	/* beginoffset */
		    !XDR_PUTLONG(&xdrs, &offset) ||	/* offset */
		    !XDR_PUTLONG(&xdrs, &tsize) ||	/* length */
		    !XDR_PUTLONG(&xdrs, &tsize)) {	/* bytes array length */
			printf("nfsdump: serialization failed\n");
			return (EIO);
		}
		m->m_len = XDR_GETPOS(&xdrs);
		m->m_next = (struct mbuf *) mclgetx(noop, 0, addr, count,
			M_DONTWAIT);
		if (m->m_next == NULL) {
			printf("nfsdump: out of mbufs\n");
			return (ENOBUFS);
		}
		error = ku_sendto_mbuf(dump_socket, m, &nfsdump_sin);
		if (error) {
			printf("nfsdump: ku_sendto_mbuf returned %d\n", error);
			return (error);
		}
retry:
		s = spl0();	/* ipintr won't run if we don't do this */
		while (dump_socket->so_rcv.sb_cc == 0) {
			if (time.tv_sec >= timeout_sec) {
				if (++timeout_count > MAXTRIES) {
					printf(
					    "nfsdump: server not responding\n");
					(void) splx(s);
					return (EIO);
				}
				(void) splx(s);
				goto resend;
			}
		}
		if (dump_socket->so_error) {
			printf("nfsdump: socket error %d\n",
				dump_socket->so_error);
			return (EIO);
		}
		m = ku_recvfrom(dump_socket, &from);
		if (m == NULL) {
			printf("nfsdump: null receive\n");
			goto retry;
		}
		s = splnet();
		sbflush(&dump_socket->so_rcv);
		(void) splx(s);
		/*
		 * Decode results.
		 */
		xdrmbuf_init(&xdrs, m, XDR_DECODE);
		reply_msg.acpted_rply.ar_verf = _null_auth;
		reply_msg.acpted_rply.ar_results.where = (caddr_t)&na;
		reply_msg.acpted_rply.ar_results.proc = xdr_attrstat;
		if (xdr_replymsg(&xdrs, &reply_msg)) {
			if (reply_msg.rm_xid != call_msg.rm_xid) {
				goto retry;
			}
			_seterr_reply(&reply_msg, &rpc_err);
			if (rpc_err.re_status == RPC_SUCCESS) {
				if (na.ns_status) {
					printf("nfs_dump. status %d\n",
						na.ns_status);
					return (EIO);
				}
				if (reply_msg.acpted_rply.ar_verf.oa_base !=
				    NULL) {
					/* free auth handle */
					xdrs.x_op = XDR_FREE;
					(void)xdr_opaque_auth(&xdrs,
					    &(reply_msg.acpted_rply.ar_verf));
				}
			} else {
				printf("nfsdump: rpc_err %d\n", rpc_err);
				return (EIO);
			}
		} else {
			printf("nfsdump: xdr_replymsg failed\n");
			return (EIO);
		}
		m_freem(m);
		/*
		 * Flush the rest of the stuff on the input queue
		 * for the socket.
		 */
		s = splnet();
		sbflush(&dump_socket->so_rcv);
		(void) splx(s);

		count -= tsize;
		addr += tsize;
		offset += tsize;
	} while (count);
	return (0);
}

static
dump_auth_marshall(xdrs)
	XDR *xdrs;
{
	register int credsize;
	register long *ptr;

	credsize = 4 + 4 + roundup(hostnamelen, 4) + 4 + 4 + 4;
	ptr = XDR_INLINE(xdrs, 4 + 4 + credsize + 4 + 4);
	if (ptr) {
		/*
		 * We can do the fast path.
		 */
		IXDR_PUT_LONG(ptr, AUTH_UNIX);	/* cred flavor */
		IXDR_PUT_LONG(ptr, credsize);	/* cred len */
		IXDR_PUT_LONG(ptr, time.tv_sec);
		IXDR_PUT_LONG(ptr, hostnamelen);
		bcopy(hostname, (caddr_t)ptr, (u_int)hostnamelen);
		ptr += roundup(hostnamelen, 4) / 4;
		IXDR_PUT_LONG(ptr, 0);		/* uid */
		IXDR_PUT_LONG(ptr, 0);		/* gid */
		IXDR_PUT_LONG(ptr, 0);		/* gid list length (empty) */
		IXDR_PUT_LONG(ptr, AUTH_NULL);	/* verf flavor */
		IXDR_PUT_LONG(ptr, 0);	/* verf len */
		return (TRUE);
	} else {
		panic("nfsdump: auth_marshall failed");
		/*NOTREACHED*/
	}
}
