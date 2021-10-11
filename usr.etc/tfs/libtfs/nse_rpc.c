#ifndef lint
static char sccsid[] = "@(#)nse_rpc.c 1.1 92/07/30 Copyr 1988 Sun Micro";
#endif

/*
 * Copyright (c) 1988 Sun Microsystems, Inc.
 */

/*
 * General routines to send RPC requests with hard retries (the request is
 * sent until the server responds,) and to create RPC client handles.
 */

#include <nse/param.h>
#include <nse/util.h>
#include <stdio.h>
#include <netdb.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <rpc/rpc.h>
#include <nse/nse_rpc.h>


#define NSE_RETRANS	3	/* Number of retransmissions for an
				 * individual request */
#define NSE_TIMEOUT	24	/* Initial timeout, in sec (should be
				 * divisible by NSE_RETRANS) */
#define NSE_MAX_TIMEOUT	300	/* Maximum timeout */

Nse_err		nse_rpc_call();
Nse_err		nse_rpc_client_create();
void		nse_rpc_client_destroy();
enum clnt_stat	nse_rpc_error();

/*
 * This is set to the specific RPC error code when the general error
 * NSE_RPC_ERROR is returned by nse_rpc_call().
 */
static enum clnt_stat last_rpc_error;

/*
 * Make RPC call numbered "procnum" to the daemon named "server" on machine
 * "host".  "create_client" is the routine used to create an RPC client
 * handle for this type of daemon, and "arg1" ... "arg3" are arguments
 * to the create routine.  This routine retries the RPC request if it
 * times out, and only returns if an error other than "timed out" occurs,
 * or if the call succeeds.
 */
Nse_err
nse_rpc_call(procnum, xdr_args, args, xdr_res, res, host, server,
	     cl_handle, create_client, arg1, arg2, arg3)
	int		procnum;
	xdrproc_t	xdr_args;
	char		*args;
	xdrproc_t	xdr_res;
	char		*res;
	char		*host;
	char		*server;
	Nse_rpc_client	cl_handle;
	Nse_err		(*create_client)();
	char		*arg1;
	char		*arg2;
	char		*arg3;
{
	struct timeval	timeout;
	int		num_tries = 0;
	bool_t		msg_printed = FALSE;
	struct rpc_err	rpc_err;
	Nse_err		err;

	timeout.tv_sec = NSE_TIMEOUT;
	timeout.tv_usec = 0;
	for (;;) {
		num_tries++;
		if (cl_handle->client == NULL ||
		    !NSE_STREQ(cl_handle->host, host)) {
			if (err = create_client(cl_handle, host,
						timeout.tv_sec / NSE_RETRANS,
						arg1, arg2, arg3, 0)) {
				return err;
			}
		}
		if (RPC_SUCCESS == clnt_call(cl_handle->client, procnum,
					     xdr_args, args,
					     xdr_res, res, timeout)) {
			if (msg_printed) {
				if (nse_get_cmdname()) {
					fprintf(stderr, "%s: ",
						nse_get_cmdname());
				}
				fprintf(stderr, "%s on %s OK\n", server, host);
				/*
				 * Set timeout back to its initial value.
				 */
#ifdef SUN_OS_4
				timeout.tv_sec = NSE_TIMEOUT / NSE_RETRANS;
				clnt_control(cl_handle->client,
					     CLSET_RETRY_TIMEOUT,
					     &timeout);
#else
				nse_rpc_client_destroy(cl_handle);
#endif SUN_OS_4
			}
			return NULL;
		}
		clnt_geterr(cl_handle->client, &rpc_err);
		nse_rpc_client_destroy(cl_handle);
		if (rpc_err.re_status != RPC_TIMEDOUT) {
			if (num_tries == 1) {
				/*
				 * Retry once if there is an error other
				 * than a timeout.
				 */
				continue;
			} else {
				last_rpc_error = rpc_err.re_status;
				return nse_err_format_rpc(NSE_RPC_ERROR,
							  server, host,
							  &rpc_err);
			}
		}
		if (nse_get_cmdname()) {
			fprintf(stderr, "%s: ", nse_get_cmdname());
		}
		fprintf(stderr, "%s on %s not responding still trying\n",
			server, host);
		msg_printed = TRUE;
		timeout.tv_sec <<= 2;
		if (timeout.tv_sec > NSE_MAX_TIMEOUT) {
			timeout.tv_sec = NSE_MAX_TIMEOUT;
		}
	}
}


Nse_err
nse_rpc_client_create(handle, host, retry_timeout, prognum, versnum,
		      server_name, port)
	Nse_rpc_client	handle;
	char		*host;
	long		retry_timeout;
	u_long		prognum;
	u_long		versnum;
	char		*server_name;
	int		port;
{
	struct hostent	*hp;
	struct timeval	timeout;

	if (handle->client) {
		nse_rpc_client_destroy(handle);
	}
	if (NSE_STREQ(host, _nse_hostname())) {
		get_myaddress(&handle->addr);
	} else {
		hp = gethostbyname(host);
		if (hp == NULL) {
			return nse_err_format_errno("Gethostbyname(%s)", host);
		}
		bcopy(hp->h_addr, (char *) &handle->addr.sin_addr,
		      hp->h_length);
	}
	handle->addr.sin_family = AF_INET;
	handle->addr.sin_port = htons((u_short) port);
	handle->sock = RPC_ANYSOCK;
	timeout.tv_sec = retry_timeout;
	timeout.tv_usec = 0;
	handle->client = clntudp_create(&handle->addr, prognum, versnum,
					timeout, &handle->sock);
	if (handle->client == NULL) {
		return nse_err_format_rpc(NSE_RPC_CANT_CONNECT, server_name,
					  host, (struct rpc_err *) NULL);
	}
	handle->host = NSE_STRDUP(host);
	return NULL;
}


void
nse_rpc_client_destroy(handle)
	Nse_rpc_client	handle;
{
	if (handle->client->cl_auth) {
		auth_destroy(handle->client->cl_auth);
	}
	clnt_destroy(handle->client);
	handle->client = NULL;
	(void) close(handle->sock);
	NSE_DISPOSE(handle->host);
}


enum clnt_stat
nse_rpc_error()
{
	return last_rpc_error;
}
