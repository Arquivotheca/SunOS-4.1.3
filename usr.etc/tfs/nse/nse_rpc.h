/*	@(#)nse_rpc.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1987 Sun Microsystems, Inc.
 */

#ifndef _NSE_RPC_H
#define _NSE_RPC_H

#include <netinet/in.h>
#include <rpc/rpc.h>
#include <nse/err.h>

/*
 * RPC error codes
 */

#define NSE_RPC_ERROR		-2201	/* Problem with an RPC call */
#define NSE_RPC_CANT_CONNECT	-2202	/* Returned if clnt_create fails */
#define NSE_RPC_BAD_PROC	-2203	/* Unknown procedure */
#define NSE_RPC_CHILD_DIED	-2204	/* Child process died */


#ifdef NSE_ERROR_COLLECT
static char *(nse_rpc_errors[]) = {
	"nse rpc",
	"-2201 -2299",
	NSE_BEGIN_ERROR_TAB,
	"Internal error: RPC problem with %s on %s: ",
	"Internal error: Cannot connect to %s on %s: ",
	"Internal error: %s: unknown procedure %d",
	"Internal error: rpc.nsed on %s for environment %s died\n  (status %d  termsig %d  coredump %d)",
	NSE_END_ERROR_TAB
};
#undef NSE_BEGIN_ERROR_TAB
#define NSE_BEGIN_ERROR_TAB (char *) nse_rpc_errors
#endif


typedef struct Nse_rpc_client {
	CLIENT		*client;
	char		*host;
	struct sockaddr_in addr;
	int		sock;
} *Nse_rpc_client, Nse_rpc_client_rec;


Nse_err		nse_rpc_call();
Nse_err		nse_rpc_client_create();
void		nse_rpc_client_destroy();
enum clnt_stat	nse_rpc_error();

#endif _NSE_RPC_H
