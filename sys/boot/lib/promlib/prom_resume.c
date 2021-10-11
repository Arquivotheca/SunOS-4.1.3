/*	@(#)prom_resume.c 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <promcommon.h>

int
prom_resumecpu(node)
	dnode_t	node;
{
	switch (obp_romvec_version)  {

	case OBP_V0_ROMVEC_VERSION:
	case OBP_V2_ROMVEC_VERSION:
		panic("prom_resumecpu");
		/* NOTREACHED */
	default:
		return (OBP_V3_RESUMECPU(node));
	}
}
