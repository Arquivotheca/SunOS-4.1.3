#ifndef lint
static        char sccsid[] = "@(#)pmap_kgetport.c 1.1 92/07/30 Copyr 1983 Sun Micro";
#endif

/*
 * pmap_kgetport.c
 * Kernel interface to pmap rpc service.
 *
 * Copyright (C) 1986, Sun Microsystems, Inc.
 */
#include <mon/sunromvec.h>
#include <rpc/types.h>
#include <netinet/in.h>
#include <rpc/xdr.h>
#include <rpc/auth.h>
#include <rpc/clnt.h>
#include <rpc/rpc_msg.h>
#include <rpc/pmap_prot.h>

#include <sys/time.h>
#include <sys/socket.h>
#include <net/if.h>

#include <sys/param.h>
#include <sys/user.h>
#include <sys/proc.h>
static struct ucred cred;
#define retries 20
static struct timeval tottimeout = { 1, 0 };

extern	int	verbosemode;



/*
 * Find the mapped port for program,version.
 * Calls the pmap service remotely to do the lookup.
 *
 * The 'address' argument is used to locate the portmapper, then
 * modified to contain the port number, if one was found.  If no
 * port number was found, 'address'->sin_port returns unchanged.
 *
 * Returns:	 0  if port number successfully found for 'program'
 *		-1  (<0) if 'program' was not registered
 *		 1  (>0) if there was an error contacting the portmapper
 */
int
pmap_kgetport(address, program, version, protocol)
	struct sockaddr_in *address;
	u_long program;
	u_long version;
	u_long protocol;
{
	u_short port = 0;
	register CLIENT *client;
	struct pmap parms;
	enum clnt_stat	status;
	struct sockaddr_in tmpaddr;

	if (verbosemode) {
		printf("Request port number from ");
		inet_print(address->sin_addr);
		printf(" for program %d version %d protocol %d\n",
			program, version, protocol);
	}

	/* copy 'address' so that it doesn't get trashed */
	tmpaddr = *address;

	tmpaddr.sin_port = htons(PMAPPORT);
	client = clntkudp_create(&tmpaddr, PMAPPROG, PMAPVERS, retries, &cred);

	if (client != (CLIENT *)NULL) {
		parms.pm_prog = program;
		parms.pm_vers = version;
		parms.pm_prot = protocol;
		parms.pm_port = 0;  /* not needed or used */
		status = CLNT_CALL(client, PMAPPROC_GETPORT, xdr_pmap, &parms,
		    xdr_u_short, &port, tottimeout);

		if (status != RPC_SUCCESS) {
			if (verbosemode) {
				printf("getport ");
				clntstat_print(status);
				printf("\n");
			}
		}

		if (verbosemode)
			printf("got port %d\n", port);


		AUTH_DESTROY(client->cl_auth);
		CLNT_DESTROY(client);
	}
	else
		printf("Boot:  getport failed:  couldn't create client handle\n");

	address->sin_port = port;	/* same the port */

	return (status);
}

#ifdef	NEVER
/*
 * getport_loop -- kernel interface to pmap_kgetport()
 *
 * Talks to the portmapper using the sockaddr_in supplied by 'address',
 * to lookup the specified 'program'.
 *
 * Modifies 'address'->sin_port by rewriting the port number, if one
 * was found.  If a port number was not found (ie, return value != 0),
 * then 'address'->sin_port is left unchanged.
 *
 * If the portmapper does not respond, prints console message (once).
 * Retries forever, unless a signal is received.
 *
 * Returns:	 0  the port number was successfully put into 'address'
 *		-1  (<0) the requested process is not registered.
 *		 1  (>0) the portmapper did not respond and a signal occurred.
 */
getport_loop(address, program, version, protocol)
	struct sockaddr_in *address;
	u_long program;
	u_long version;
	u_long protocol;
{
	register int pe = 0;
	register int i = 0;

	/* sit in a tight loop until the portmapper responds */
	while ((i = pmap_kgetport(address, program, version, protocol)) > 0) {

		/* test to see if a signal has come in */
		if (ISSIG(u.u_procp)) {
			printf("Portmapper not responding; giving up\n");
			goto out;		/* got a signal */
		}
		/* print this message only once */
		if (pe++ == 0) {
			printf("Portmapper not responding; still trying\n");
		}
	}				/* go try the portmapper again */

	/* got a response...print message if there was a delay */
	if (pe != 0) {
		printf("Portmapper ok\n");
	}
out:
	return(i);	/* may return <0 if program not registered */
}
#endif	/* NEVER */
