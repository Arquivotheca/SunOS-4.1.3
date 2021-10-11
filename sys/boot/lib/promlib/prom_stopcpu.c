/*	@(#)prom_stopcpu.c 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <promcommon.h>

int
prom_stopcpu(node)
	dnode_t		node;
{
	switch (obp_romvec_version)  {

	case OBP_V0_ROMVEC_VERSION:
	case OBP_V2_ROMVEC_VERSION:
		panic("prom_stopcpu");
		/* NOTREACHED */
	default:
		/* XXX: flush_windows? */
		return (OBP_V3_STOPCPU(node));
	}
}
