/*	@(#)prom_setprop.c 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <promcommon.h>

int
prom_setprop(nodeid, name, value, len)
	dnode_t nodeid;
	caddr_t name;
	caddr_t value;
	int	len;
{
	return (OBP_DEVR_SETPROP(nodeid, name, value, len));
}
