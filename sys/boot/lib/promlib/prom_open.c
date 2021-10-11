/*	@(#)prom_open.c 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <promcommon.h>

int
prom_open(name)
	char	*name;
{
	int i;
	switch (obp_romvec_version)  {

	case OBP_V0_ROMVEC_VERSION:
		if ((i = (int)(OBP_V0_OPEN(name))) == 0)
			return (-1);
		return(i);

	default:
		return (OBP_V2_OPEN(name));
	}
}
