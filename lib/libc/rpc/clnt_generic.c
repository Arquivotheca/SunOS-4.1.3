#if !defined(lint) && defined(SCCSIDS)
static char sccsid[] = "@(#)clnt_generic.c 1.1 92/07/30 (C) 1987 SMI";
#endif
/*
 * Copyright (C) 1987, Sun Microsystems, Inc.
 */
#include <rpc/rpc.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/errno.h>
#include <netdb.h>

static CLIENT * _rpc_clnt_create_both();
/*
 * Generic client creation: takes (hostname, program-number, protocol) and
 * returns client handle. Default options are set, which the user can
 * change using the rpc equivalent of ioctl()'s.
 */

CLIENT *
clnt_create(hostname, prog, vers, proto)
	char *hostname;
	unsigned prog;
	unsigned vers;
	char *proto;
{
	unsigned vers_out;
	return (_rpc_clnt_create_both(hostname, prog, &vers_out, vers,
		vers, proto, FALSE));

}

/*
 * Generic client creation with version checking the value of
 * vers_out is set to the highest server supported value
 * vers_low <= vers_out <= vers_high  AND an error results
 * if this can not be done. Mount uses this.
 */

CLIENT *
clnt_create_vers(hostname, prog, vers_out, vers_low, vers_high,
	proto)
	char *hostname;
	unsigned prog;
	unsigned *vers_out;
	unsigned vers_low;
	unsigned vers_high;
	char *proto;
{
	return (_rpc_clnt_create_both(hostname, prog, vers_out, vers_low,
		vers_high, proto, TRUE));
}

/*
 * Generic client creation: takes (hostname, program-number, protocol) and
 * returns client handle. Default options are set, which the user can
 * change using the rpc equivalent of ioctl()'s.
 */
static CLIENT *
_rpc_clnt_create_both(hostname, prog, vers_out, vers_low, vers_high,
	proto, doversions)
	char *hostname;
	unsigned prog;
	unsigned *vers_out;
	unsigned vers_low;
	unsigned vers_high;
	char *proto;
	int  doversions;
{
	struct hostent *h;
	struct protoent *p;
	struct sockaddr_in sin;
	int sock;
	struct timeval tv, to;
	int minvers, maxvers;
	struct rpc_err rpcerr;
	int must_work;
	enum clnt_stat rpc_stat;
	unsigned long inet_addr();

	CLIENT *client;


	sin.sin_family = AF_INET;
	sin.sin_port = 0;
	bzero(sin.sin_zero, sizeof (sin.sin_zero));
	sin.sin_addr.s_addr = inet_addr(hostname);
	if ((int) (sin.sin_addr.s_addr) == -1) {

		h = gethostbyname(hostname);
		if (h == NULL) {
			rpc_createerr.cf_stat = RPC_UNKNOWNHOST;
			return (NULL);
		}
		if (h->h_addrtype != AF_INET) {
			rpc_createerr.cf_stat = RPC_SYSTEMERROR;
			rpc_createerr.cf_error.re_errno = EAFNOSUPPORT;
			return (NULL);
		}
		bcopy(h->h_addr, (char *) &sin.sin_addr, h->h_length);
		sin.sin_family = AF_INET;
		sin.sin_port = 0;
	}
	p = getprotobyname(proto);
	if (p == NULL) {
		rpc_createerr.cf_stat = RPC_UNKNOWNPROTO;
		rpc_createerr.cf_error.re_errno = EPFNOSUPPORT;
		return (NULL);
	}
	for (must_work = 0; must_work <= 1; must_work++) {

		sock = RPC_ANYSOCK;

		switch (p->p_proto) {
		case IPPROTO_UDP:
			tv.tv_sec = 5;
			tv.tv_usec = 0;
			client = clntudp_create(&sin, prog,
				vers_high, tv, &sock);
			if (client == NULL) {
				return (NULL);
			}
			tv.tv_sec = 25;
			clnt_control(client, CLSET_TIMEOUT, &tv);
			break;
		case IPPROTO_TCP:
			client = clnttcp_create(&sin, prog,
				vers_high, &sock, 0, 0);
			if (client == NULL) {
				return (NULL);
			}
			tv.tv_sec = 25;
			tv.tv_usec = 0;
			clnt_control(client, CLSET_TIMEOUT, &tv);
			break;
		default:
			rpc_createerr.cf_stat = RPC_SYSTEMERROR;
			rpc_createerr.cf_error.re_errno = EPFNOSUPPORT;
			return (NULL);
		}

		if (doversions == 0) return (client);

		to.tv_sec = 10;
		to.tv_usec = 0;
		rpc_stat = clnt_call(client, NULLPROC, xdr_void, (char *) NULL,
		    xdr_void, (char *) NULL, to);
		if (rpc_stat == RPC_SUCCESS) {
			*vers_out = vers_high;
			return (client);
		} else if ((must_work == 0) &&
				(rpc_stat == RPC_PROGVERSMISMATCH)) {
			clnt_geterr(client, &rpcerr);
			minvers = rpcerr.re_vers.low;
			maxvers = rpcerr.re_vers.high;
			if (maxvers < vers_high)
				vers_high = maxvers;
			if (minvers > vers_low)
				vers_low = minvers;
			if (vers_low > vers_high) {
				rpc_createerr.cf_stat = rpc_stat;
				rpc_createerr.cf_error = rpcerr;
				clnt_destroy(client);
				return (NULL);
			}
			clnt_destroy(client);
		} else {
			clnt_geterr(client, &rpcerr);
			rpc_createerr.cf_stat = rpc_stat;
			rpc_createerr.cf_error = rpcerr;
			clnt_destroy(client);
			return (NULL);
		}



	}
	return (client);
}
