/*	@(#)prom_phandle.c 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <promcommon.h>

phandle_t
prom_getphandle(i)
	ihandle_t	i;
{
	switch (obp_romvec_version)  {

	case OBP_V0_ROMVEC_VERSION:
		return ((phandle_t)(OBP_BADNODE));

	default:
		return (OBP_V2_PHANDLE(i));
	}
}
