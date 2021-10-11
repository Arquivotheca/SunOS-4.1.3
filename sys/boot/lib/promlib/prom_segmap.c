/*	@(#)prom_segmap.c 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <promcommon.h>

/*
 * sun4/sun4c (Sun MMU) only!
 */

void
prom_setcxsegmap(c, v, seg)
	int c;
	addr_t v;
	u_char seg;
{
	OBP_SETCXSEGMAP(c, v, seg);
	return;
}
