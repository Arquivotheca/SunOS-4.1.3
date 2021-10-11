/*	@(#)prom_gettime.c 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <promcommon.h>

u_int
prom_gettime()
{
	return (OBP_MILLISECONDS);
}
