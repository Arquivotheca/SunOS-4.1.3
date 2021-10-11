/*	@(#)prom_startcpu.c 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <promcommon.h>

int
prom_startcpu(node, context, whichcontext, pc)
	dnode_t		node;
	struct dev_reg	*context;
	int		whichcontext;
	addr_t		pc;
{
	switch (obp_romvec_version)  {

	case OBP_V0_ROMVEC_VERSION:
	case OBP_V2_ROMVEC_VERSION:
		panic("prom_startcpu");
		/* NOTREACHED */

	default:
		return (OBP_V3_STARTCPU(node, context, whichcontext, pc));
	}
}
