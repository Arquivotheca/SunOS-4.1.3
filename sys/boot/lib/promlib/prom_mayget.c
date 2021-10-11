/*	@(#)prom_mayget.c 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <promcommon.h>

int
prom_mayget()
{
	char c;

	switch (obp_romvec_version)  {

	case OBP_V0_ROMVEC_VERSION:
		return (OBP_V0_MAYGET());
	default:
		if (OBP_V2_READ(OBP_V2_STDIN, &c, 1) == 1)
			return ((int)c);
		return (-1);
	}
}
