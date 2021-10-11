/*	@(#)pmap_prot.h 1.1 92/07/30 SMI	*/

/*
 * pmap_prot.h
 * Protocol for the local binder service, or pmap.
 *
 * Copyright (C) 1984, Sun Microsystems, Inc.
 *
 * The following procedures are supported by the protocol:
 *
 * PMAPPROC_NULL() returns ()
 * 	takes nothing, returns nothing
 *
 * PMAPPROC_SET(struct pmap) returns (bool_t)
 * 	TRUE is success, FALSE is failure.  Registers the tuple
 *	[prog, vers, prot, port].
 *
 * PMAPPROC_UNSET(struct pmap) returns (bool_t)
 *	TRUE is success, FALSE is failure.  Un-registers pair
 *	[prog, vers].  prot and port are ignored.
 *
 * PMAPPROC_GETPORT(struct pmap) returns (long unsigned).
 *	0 is failure.  Otherwise returns the port number where the pair
 *	[prog, vers] is registered.  It may lie!
 *
 * PMAPPROC_DUMP() RETURNS (struct pmaplist *)
 *
 * PMAPPROC_CALLIT(unsigned, unsigned, unsigned, string<>)
 * 	RETURNS (port, string<>);
 * usage: encapsulatedresults =
 *		PMAPPROC_CALLIT(prog, vers, proc, encapsulatedargs);
 * 	Calls the procedure on the local machine.  If it is not registered,
 *	this procedure is quite; ie it does not return error information!!!
 *	This procedure only is supported on rpc/udp and calls via
 *	rpc/udp.  This routine only passes null authentication parameters.
 *	This file has no interface to xdr routines for PMAPPROC_CALLIT.
 *
 * The service supports remote procedure calls on udp/ip or tcp/ip socket 111.
 */

#ifndef _rpc_pmap_prot_h
#define	_rpc_pmap_prot_h

#define	PMAPPORT		((u_short)111)
#define	PMAPPROG		((u_long)100000)
#define	PMAPVERS		((u_long)2)
#define	PMAPVERS_PROTO		((u_long)2)
#define	PMAPVERS_ORIG		((u_long)1)
#define	PMAPPROC_NULL		((u_long)0)
#define	PMAPPROC_SET		((u_long)1)
#define	PMAPPROC_UNSET		((u_long)2)
#define	PMAPPROC_GETPORT	((u_long)3)
#define	PMAPPROC_DUMP		((u_long)4)
#define	PMAPPROC_CALLIT		((u_long)5)

struct pmap {
	long unsigned pm_prog;
	long unsigned pm_vers;
	long unsigned pm_prot;
	long unsigned pm_port;
};

extern bool_t xdr_pmap();

struct pmaplist {
	struct pmap	pml_map;
	struct pmaplist *pml_next;
};

#ifndef KERNEL
extern bool_t xdr_pmaplist();
#endif /*!KERNEL*/

#endif /*!_rpc_pmap_prot_h*/
