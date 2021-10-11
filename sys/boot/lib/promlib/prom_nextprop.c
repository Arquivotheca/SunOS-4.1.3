/*	@(#)prom_nextprop.c 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <promcommon.h>

/*
 * Caller must  check for OBP_NULLPROP/OBP_BADPROP return cookie values.
 */

caddr_t
prom_nextprop(nodeid, previous)
	dnode_t nodeid;
	caddr_t previous;
{
	return (OBP_DEVR_NEXTPROP(nodeid, previous));
}
