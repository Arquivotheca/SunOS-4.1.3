/*	@(#)prom_map.c 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <promcommon.h>

caddr_t
prom_map(virthint, space, phys, size)
	caddr_t	virthint;
	u_int	space, phys, size;
{
	switch (obp_romvec_version)  {

	case OBP_V0_ROMVEC_VERSION:
		return ((char *)0);

	default:
		return (OBP_V2_MAP(virthint, space, phys, size));

	}
}
