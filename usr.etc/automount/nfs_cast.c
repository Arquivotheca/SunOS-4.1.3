#ifndef lint
static  char sccsid[] = "@(#)nfs_cast.c 1.1 92/07/30 Copyr 1989 Sun Micro";
#endif

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */


/*
 * nfs_cast: broadcast to a specific group of NFS servers
 */

#include <syslog.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netdb.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <rpc/rpc.h>
#include <netinet/in.h>
#include <nfs/nfs.h>

extern int errno;

typedef bool_t (*resultproc_t)();

enum clnt_stat 
nfs_cast(addrs, eachresult, timeout)
	struct in_addr *addrs;		/* list of host addresses */
	resultproc_t	eachresult;	/* call with each result obtained */
	int		timeout;	/* timeout (sec) */
{
	enum clnt_stat stat;
	AUTH *unix_auth = authunix_create_default();
	XDR xdr_stream;
	register XDR *xdrs = &xdr_stream;
	int outlen, inlen, fromlen;
	int sent;
	int sock;
	fd_set readfds, mask;
	bool_t done = FALSE;
	register u_long xid;		/* xid - unique per addr */
	u_long port;
	struct sockaddr_in baddr;	/* "to" address */
	struct sockaddr_in raddr;	/* "from" address (ignored) */
	register int i;
	struct rpc_msg msg;
	struct timeval t; 
	char outbuf[UDPMSGSIZE], inbuf[UDPMSGSIZE];

	/*
	 * initialization: create a socket, a broadcast address, and
	 * preserialize the arguments into a send buffer.
	 */

	if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
		syslog(LOG_ERR, "Cannot create socket for nfs: %m");
		stat = RPC_CANTSEND;
		goto done_broad;
	}
	FD_ZERO(&mask);
	FD_SET(sock, &mask);
	memset((char *)&baddr, 0, sizeof (baddr));
	baddr.sin_family = AF_INET;
	baddr.sin_port = htons(NFS_PORT);
	(void) gettimeofday(&t, (struct timezone *) 0);
	xid = (getpid() ^ t.tv_sec ^ t.tv_usec) & ~0xFF;
	t.tv_usec = 0;

	/* serialize the RPC header */
	msg.rm_direction = CALL;
	msg.rm_call.cb_rpcvers = RPC_MSG_VERSION;
	msg.rm_call.cb_prog = NFS_PROGRAM;
	msg.rm_call.cb_vers = NFS_VERSION;
	msg.rm_call.cb_proc = NULLPROC;
	if (unix_auth == (AUTH *) NULL) {
		stat = RPC_SYSTEMERROR;
		goto done_broad;
	}
	msg.rm_call.cb_cred = unix_auth->ah_cred;
	msg.rm_call.cb_verf = unix_auth->ah_verf;
	xdrmem_create(xdrs, outbuf, sizeof outbuf, XDR_ENCODE);
	if (! xdr_callmsg(xdrs, &msg)) {
		stat = RPC_CANTENCODEARGS;
		goto done_broad;
	}
	outlen = (int)xdr_getpos(xdrs);
	xdr_destroy(xdrs);

	/*
	 * Basic loop: send packet to all hosts and wait for response(s).
	 * The response timeout grows larger per iteration.
	 * A unique xid is assigned to each request in order to
	 * correctly match the replies.
	 */
	for (t.tv_sec = 4; timeout > 0; t.tv_sec += 2) {
		timeout -= t.tv_sec;
		if (timeout < 0)
			t.tv_sec = timeout - t.tv_sec;
		sent = 0;
		for (i = 0; addrs[i].s_addr != 0; i++) {
			baddr.sin_addr = addrs[i];
			/* xid is first thing in preserialized buffer */
			*((u_long *)outbuf) = htonl(xid + i);
			if (sendto(sock, outbuf, outlen, 0,
			    (struct sockaddr *)&baddr,
			    sizeof (struct sockaddr)) != outlen) {
				syslog(LOG_ERR,
				    "nfscast: Cannot send packet: %m");
				continue;
			}
			sent++;
		}
		if (sent == 0) {		/* no packets sent ? */
			stat = RPC_CANTSEND;
			goto done_broad;
		}

		/*
		 * Have sent all the packets.  Now collect the responses...
		 */
	recv_again:
		msg.acpted_rply.ar_verf = _null_auth;
		msg.acpted_rply.ar_results.proc = xdr_void;
		readfds = mask;
		switch (select(32, &readfds, (fd_set *) NULL, (fd_set *) NULL, &t)) {

		case 0:  /* timed out */
			stat = RPC_TIMEDOUT;
			continue;

		case -1:  /* some kind of error */
			if (errno == EINTR)
				goto recv_again;
			syslog(LOG_ERR, "nfscast: select: %m");
			stat = RPC_CANTRECV;
			goto done_broad;

		}  /* end of select results switch */

		if (!FD_ISSET(sock, &readfds))
			goto recv_again;

	try_again:
		fromlen = sizeof(struct sockaddr);
		inlen = recvfrom(sock, inbuf, sizeof inbuf, 0,
			(struct sockaddr *)&raddr, &fromlen);
		if (inlen < 0) {
			if (errno == EINTR)
				goto try_again;
			syslog(LOG_ERR,
			    "nfscast: cannot receive reply: %m");
			stat = RPC_CANTRECV;
			goto done_broad;
		}
		if (inlen < sizeof(u_long))
			goto recv_again;
		/*
		 * see if reply transaction id matches sent id.
		 * If so, decode the results.
		 * Note: received addr is ignored, it could be different
		 * from the send addr if the host has more than one addr.
		 */
		xdrmem_create(xdrs, inbuf, inlen, XDR_DECODE);
		if (xdr_replymsg(xdrs, &msg)) {
			if (((msg.rm_xid & ~0xFF) == xid) &&
				(msg.rm_reply.rp_stat == MSG_ACCEPTED) &&
				(msg.acpted_rply.ar_stat == SUCCESS)) {
				raddr.sin_port = htons((u_short) port);
				raddr.sin_addr = addrs[msg.rm_xid - xid];
				done = (*eachresult)(&raddr);
			}
			/* otherwise, we just ignore the errors ... */
		}
		xdrs->x_op = XDR_FREE;
		msg.acpted_rply.ar_results.proc = xdr_void;
		(void)xdr_replymsg(xdrs, &msg);
		XDR_DESTROY(xdrs);
		if (done) {
			stat = RPC_SUCCESS;
			goto done_broad;
		} else {
			goto recv_again;
		}
	}
	stat = RPC_TIMEDOUT;

done_broad:
	(void) close(sock);
	AUTH_DESTROY(unix_auth);
	return (stat);
}
