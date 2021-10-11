#ident "@(#)espdma.c 1.1 92/07/30 SMI"

/*
 * Copyright (c) 1988, 1989, 1990 by Sun Microsystems, Inc.
 */
#include "dma.h"
#if	NDMA > 0
/*
 * Sbus DMA gate array 'driver'
 */

#include <sys/param.h>
#include <sys/buf.h>
#include <sys/systm.h>

#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/debug.h>
#include <sundev/mbvar.h>

#include <sun/autoconf.h>
#include <sbusdev/dmaga.h>
#include <machine/mmu.h>

static int espdmaintr();

extern int ndma_map;
extern struct espdma_map {
	struct dmaga *regs;
	int bustype;
	addr_t addr;
	char free;
} *dma_map;

static int espdmaidentify(), espdmaattach();
struct	dev_ops espdma_ops = {
	1,
	espdmaidentify,
	espdmaattach
};

static int
espdmaidentify(name)
char *name;
{
	if (strcmp(name, "espdma") == 0) {
		ndma_map++;
		return (1);
	} else
		return (0);
}
/*
 *
 */
static int
espdmaattach(dev)
	register struct dev_info *dev;
{
	register struct espdma_map *dp;
	static char unit_no = 0;


	if (!dma_map) {
		dma_map = (struct espdma_map *)
		    kmem_alloc((u_int) (sizeof (struct espdma_map) * ndma_map));
		if (!dma_map) {
			printf("espdma%d: No space for dma_map structure\n",
			    dev->devi_unit);
			return (-1);
		}
	}

	if ((dev->devi_unit = unit_no++) >= ndma_map) {
		printf("espdma%d: bad unit number\n", dev->devi_unit);
		return (-1);
	} else
		dp = &dma_map[dev->devi_unit];

	if (dev->devi_nreg > 1) {
		printf("espdma%d: bad register specification\n", dev->devi_unit);
		return (-1);
	}


	/* map in the device registers */
	dp->regs = (struct dmaga *)
	    map_regs(dev->devi_reg->reg_addr, dev->devi_reg->reg_size,
		dev->devi_reg->reg_bustype);
	if (dp->regs == (struct dmaga *) 0) {
		printf("espdma%d: unable to map registers\n", dev->devi_unit);
		return (-1);
	}
	dp->bustype = dev->devi_reg->reg_bustype;
	dp->addr = dev->devi_reg->reg_addr;
	dp->free = 1;
	if (dev->devi_nintr) {
		addintr(dev->devi_intr->int_pri, espdmaintr,
		    dev->devi_name, dev->devi_unit);
	}
	report_dev(dev);
	attach_devs(dev);
	return (0);
}

static int
espdmaintr()
{
	return (0);
}

#endif	NDMA > 0
