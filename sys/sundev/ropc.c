#ifndef lint
static char sccsid[] = "@(#)ropc.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright 1983, 1987 by Sun Microsystems, Inc.
 */

/*
 *  Sun-2 Rasterop Chip Driver
 */
#include "ropc.h"

#include <sys/param.h>
#include <sys/buf.h>
#include <sys/errno.h>
#include <sundev/mbvar.h>
#include <pixrect/memreg.h>

/*
 * Driver information for auto-configuration stuff.
 */
int ropcprobe();
struct mb_device *ropcinfo[1];	/* XXX only supports 1 chip */
struct mb_driver ropcdriver = {
	ropcprobe, 0, 0, 0, 0, 0,
	2 * sizeof(struct memropc), "ropc", ropcinfo, 0, 0, 0,
};

struct memropc *mem_ropc;

/*ARGSUSED*/
ropcprobe(reg, unit)
	caddr_t reg;
	int unit;
{
	register struct memropc *ropcaddr = (struct memropc *) reg;

	if (peek((short *) ropcaddr) == -1 ||
		poke((short *) &ropcaddr->mrc_x15, 0x13) || 
		peek((short *) &ropcaddr->mrc_x15) != 0xff13)
		return 0;

	mem_ropc = ropcaddr;

	return sizeof *ropcaddr;
}

/*ARGSUSED*/
ropcopen(dev, flag)
	dev_t dev;
	int flag;
{
	return mem_ropc ? 0 : ENXIO;
}

/*ARGSUSED*/
ropcmmap(dev, off, prot)
	dev_t dev;
	off_t off;
	int prot;
{
	return off ? -1 : fbgetpage((caddr_t) mem_ropc);
}
