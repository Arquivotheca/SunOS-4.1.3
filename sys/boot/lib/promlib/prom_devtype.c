/*	@(#)prom_devtype.c 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <promcommon.h>

int
prom_devicetype(id, type)
	dnode_t id;
	char *type;
{
	register int len;
	char buf[128];

	len = prom_getproplen(id, OBP_DEVICETYPE);
	if (len <= 0 || len >= (sizeof buf))
		return (0);

	(void)prom_getprop(id, OBP_DEVICETYPE, buf);

	if (strcmp(type, buf) == 0)
		return (1);

	return (0);
}
