/*	@(#)prom_getprop.c 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <promcommon.h>

int
prom_getproplen(nodeid, name)
	dnode_t nodeid;
	caddr_t name;
{
	return (OBP_DEVR_GETPROPLEN(nodeid, name));
}

int
prom_getprop(nodeid, name, value)
	dnode_t nodeid;
	caddr_t name;
	caddr_t value;
{
	return (OBP_DEVR_GETPROP(nodeid, name, value));
}
