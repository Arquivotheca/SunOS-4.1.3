/*	@(#)prom_mayput.c 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <promcommon.h>

int
prom_mayput(c)
	char c;
{
	switch (obp_romvec_version)  {

	case OBP_V0_ROMVEC_VERSION:
		return (OBP_V0_MAYPUT(c));
	default:
		if (OBP_V2_WRITE(OBP_V2_STDOUT, &c, 1) == 1)
			return (0);
		return (-1);
	}
}
