/*	@(#)prom_alloc.c 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <promcommon.h>

caddr_t
prom_alloc(virthint, size)
	caddr_t	virthint;
	u_int	size;
{
	switch (obp_romvec_version)  {

	case OBP_V0_ROMVEC_VERSION:
		return ((char *)0);

	default:
		return (OBP_V2_ALLOC(virthint, size));

	}
}
