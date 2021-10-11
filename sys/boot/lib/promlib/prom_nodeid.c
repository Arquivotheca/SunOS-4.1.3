/*	@(#)prom_nodeid.c 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <promcommon.h>

dnode_t
prom_nextnode(nodeid)
	dnode_t  nodeid;
{
	return (OBP_DEVR_NEXT(nodeid));
}

dnode_t
prom_childnode(nodeid)
	dnode_t nodeid;
{
	return (OBP_DEVR_CHILD(nodeid));
}
