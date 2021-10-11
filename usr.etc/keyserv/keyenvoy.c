#ifndef lint
static char sccsid[] = "@(#)keyenvoy.c 1.1 92/07/30 Copyr 1986 Sun Micro";
#endif
/*
 * Copyright (C) 1986, Sun Microsystems, Inc.
 */
#include <stdio.h>
#include <rpc/rpc.h>
#include <rpc/key_prot.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <fcntl.h>

/*
 * Talk to the keyserver on a privileged port on the part of a calling program.
 *
 * Protocol is for caller to send through stdin the procedure number 
 * to call followed by the argument data.  We call the keyserver, and
 * send the results back to the caller through stdout.
 * Non-zero exit status means something went wrong.
 */

#ifndef DEBUG
#define debug(msg)	
#endif

#define TOTAL_TIMEOUT 30	/* total timeout talking to keyserver */
#define TOTAL_TRIES	  10	/* Number of tries */

/*
 * Opaque data that we send and receive
 */
#define MAXOPAQUE 256
struct opaqn {
	u_int len;	
	u_int data[MAXOPAQUE];
};
bool_t xdr_opaqn();


main(argc,argv)
	int argc;
	char *argv[];
{
	XDR xdrs_args;
	XDR xdrs_rslt;
	int proc;
	struct opaqn args, rslt;


	if (isatty(0)) {
		fprintf(stderr, 
			"This program cannot be used interactively.\n");
		exit(1);
	}

#ifdef DEBUG
	close(2);
	open("/dev/console", O_WRONLY, 0);
#endif

	xdrstdio_create(&xdrs_args, stdin, XDR_DECODE);
	xdrstdio_create(&xdrs_rslt, stdout, XDR_ENCODE);

	if ( ! xdr_u_long(&xdrs_args, &proc)) {
		debug("no proc");
		exit(1);
	}
	if (! xdr_opaqn(&xdrs_args, &args)) {
		debug("recving args failed");
		exit(1);
	}	
	if (! callkeyserver(proc, xdr_opaqn, &args, xdr_opaqn, &rslt)) {
		debug("rpc_call failed");
		exit(1);
	}
	if (! xdr_opaqn(&xdrs_rslt, &rslt)) {
		debug("sending args failed");
		exit(1);
	}
	exit(0);
	/* NOTREACHED */
}



callkeyserver(proc, xdr_args, args, xdr_rslt, rslt)
	u_long proc;
	bool_t (*xdr_args)();
	void *args;
	bool_t (*xdr_rslt)();
	void *rslt;

{
	struct sockaddr_in remote;
	int port;
	struct timeval wait;
	enum clnt_stat stat;
	CLIENT *client;
	int sd;

	/*
  	 * set up the remote address
	 * and create client
	 */
	remote.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	remote.sin_family = AF_INET;
	remote.sin_port = 0;
	wait.tv_sec = TOTAL_TIMEOUT/TOTAL_TRIES; wait.tv_usec = 0;
	sd = RPC_ANYSOCK;
	client = clntudp_create(&remote, KEY_PROG, KEY_VERS, wait, &sd);
	if (client == NULL) {
		debug("no client");
		return (0);
	}

	/*
	 * Check that server is bound to a reserved port, so
	 * that noone can masquerade as the keyserver.
	 */
	if (ntohs(remote.sin_port) >= IPPORT_RESERVED) {
		debug("insecure port");
		return (0);
	}

	/*
	 * Create authentication
	 * All we care about really is sending the real uid
	 */
	client->cl_auth = authunix_create("", getuid(), 0, 0, NULL);
	if (client->cl_auth == NULL) {
		debug("no auth");
		return (0);
	}
	wait.tv_sec = TOTAL_TIMEOUT; wait.tv_usec = 0;
	stat = clnt_call(client, proc, xdr_args, args, xdr_rslt, rslt, wait); 
	if (stat != RPC_SUCCESS) {
		debug("clnt_call failed");
	}
	return (stat == RPC_SUCCESS);
}


/*
 * XDR opaque data
 * Don't know the length on decode, so just keep receiving until failure.
 */ 
bool_t
xdr_opaqn(xdrs, objp)
	XDR *xdrs;
	struct opaqn *objp;
{
	int i;

	switch (xdrs->x_op) {
	case XDR_FREE:
		break;	
	case XDR_DECODE:
		for (i = 0; i < MAXOPAQUE &&  xdr_int(xdrs, &objp->data[i]); i++) {
		}
		if (i == MAXOPAQUE) {
			return (FALSE);
		}
		objp->len = i;
		break;	
	case XDR_ENCODE:
		for (i = 0; i < objp->len; i++) {
			if (! xdr_int(xdrs, &objp->data[i])) {
				return (FALSE);
			}
		}
		break;
	}
	return (TRUE);
}


#ifdef DEBUG
debug(msg)
     char *msg;
{
  fprintf(stderr, "%s\n", msg);
}
#endif
