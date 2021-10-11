/*
 * @(#)prom_mayput.c 1.1 92/07/30 Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */


#include <mon/openprom.h>

#ifdef OPENPROMS
extern	int	obp_romvec_version;

int
prom_mayput(c)
	char c;
{
	register int	fd;

	switch (obp_romvec_version)  {

	case OBP_V0_ROMVEC_VERSION:
		return (OBP_V0_MAYPUT(c));
	default:
		fd = (*romp->op2_stdout);
		if (OBP_V2_WRITE(fd, &c, 1) == 1)
			return (0);
		return (-1);
	}
}
#endif OPENPROMS
