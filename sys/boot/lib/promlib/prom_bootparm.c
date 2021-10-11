/*	@(#)prom_bootparm.c 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <promcommon.h>

struct bootparam *
prom_bootparam()
{
	switch (obp_romvec_version)  {

	case OBP_V0_ROMVEC_VERSION:
		return (OBP_V0_BOOTPARAM);

	default:
		return ((struct bootparam *)0);
	}
}
