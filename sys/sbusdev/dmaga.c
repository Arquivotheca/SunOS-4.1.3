#ident	"@(#)dmaga.c	1.1"
/* @(#)dmaga.c 1.1 92/07/30 SMI */
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

static int dmaintr();

int ndma_map = 0;
struct dma_map {
	struct dmaga *regs;
	int bustype;
	addr_t addr;
	char free;
} *dma_map;

static int dmaidentify(), dmaattach();
struct	dev_ops dma_ops = {
	1,
	dmaidentify,
	dmaattach
};

#ifdef	GASP
/*
 * This code for GASP cards (that have no prom)
 */

/*
 * GASP cards can be in slot 1, slot 1 + 0x1000000,
 * slot 2, slot 2 + 0x2000000
 */

int gasp_cards = 0;

#define	GASP_OFFSET	0x1000000
u_long gasp_addrs[4] = {
	SBUS_BASE+SBUS_SIZE,
	SBUS_BASE+SBUS_SIZE+GASP_OFFSET,
	SBUS_BASE+(SBUS_SIZE<<1),
	SBUS_BASE+(SBUS_SIZE<<1)+GASP_OFFSET
};


#include <machine/pte.h>

#define	DMA_OFFSET	0x400000
#define	Dma_addr(card)	(gasp_addrs[(card)] + DMA_OFFSET)

static void
new_dma(prev, gaddr)
struct dev_info *prev;
u_long gaddr;
{
	struct dev_info *dev;
	auto long tmp;

	dev = (struct dev_info *) kmem_zalloc (sizeof (*dev));
	dev->devi_parent =  prev->devi_parent;
	dev->devi_next = prev->devi_next;
	prev->devi_next = dev;
	dev->devi_name = prev->devi_name;
	dev->devi_nreg = 1;
	dev->devi_reg = (struct dev_reg *)
		kmem_zalloc(sizeof (struct dev_reg));
	dev->devi_reg->reg_bustype = OBIO;
	dev->devi_reg->reg_addr = (addr_t) gaddr;
	dev->devi_reg->reg_size = sizeof (struct dmaga);
	dev->devi_driver = &dma_ops;
	dev->devi_nodeid = prev->devi_nodeid; /* lies. all lies */
#if defined(DEVINFO_DEBUG)
printf("new_dma() constructed a devinfo node\n");
#endif
	ndma_map += 1;

}
#endif	GASP


static int
dmaidentify(name)
char *name;
{
	if (strcmp(name, "dma") == 0) {
		ndma_map++;
		return (1);
	} else
		return (0);
}
/*
 *
 */
static int
dmaattach(dev)
	register struct dev_info *dev;
{
	register struct dma_map *dp;
	static char unit_no = 0;

#ifdef	GASP
	if (unit_no == 0) {
		register card;
		struct dev_info *tmp = dev;
		for (card = 0; card < 4; card++) {
			if (gasp_cards & (1<<card)) {
				new_dma(tmp, Dma_addr(card));
				tmp = tmp->devi_next;
			}
		}
	}
#endif	GASP

	if (!dma_map) {
		dma_map = (struct dma_map *)
		    kmem_alloc((u_int) (sizeof (struct dma_map) * ndma_map));
		if (!dma_map) {
			printf("dma%d: No space for dma_map structure\n",
			    dev->devi_unit);
			return (-1);
		}
	}

	if ((dev->devi_unit = unit_no++) >= ndma_map) {
		printf("dma%d: bad unit number\n", dev->devi_unit);
		return (-1);
	} else
		dp = &dma_map[dev->devi_unit];

	if (dev->devi_nreg > 1) {
		printf("dma%d: bad register specification\n", dev->devi_unit);
		return (-1);
	}


	/* map in the device registers */
	dp->regs = (struct dmaga *)
	    map_regs(dev->devi_reg->reg_addr, dev->devi_reg->reg_size,
		dev->devi_reg->reg_bustype);
	if (dp->regs == (struct dmaga *) 0) {
		printf("dma%d: unable to map registers\n", dev->devi_unit);
		return (-1);
	}
	dp->bustype = dev->devi_reg->reg_bustype;
	dp->addr = dev->devi_reg->reg_addr;
	dp->free = 1;
	if (dev->devi_nintr) {
		addintr(dev->devi_intr->int_pri, dmaintr,
		    dev->devi_name, dev->devi_unit);
	}
	report_dev(dev);
	attach_devs(dev);
#ifdef	GASP
	if (dev->devi_unit == 0 && ndma_map > 1) {
		int slot;
		for (slot = 1; slot < ndma_map; slot++) {
			dev = dev->devi_next;
			(void) dmaattach(dev);
		}
	}
#endif	GASP
	return (0);
}

#ifdef SBUS_SIZE
/*
 * If SBUS_SIZE is provided, then we just consider the PArange as
 * divided up into hunks the size of the S-Bus, and this macro returns
 * which one it is.  It may not really correspond to the slot, but is
 * OK for use in dma_alloc.  Otherwise, a function sbusslot() must be
 * available that gives the slot number from the low 32 bits of the
 * paddr.
 */
#define	sbusslot(pa)	(((int)(pa))/SBUS_SIZE)
#endif

struct dmaga *
dma_alloc(bustype, addr)
	register int bustype;
	register addr_t addr;
{
	register i;
	register struct dma_map *dp = dma_map;

	for (i = 0; i < ndma_map; i++) {
		if ((dp->free && (dp->bustype == bustype)) &&
		    (sbusslot((unsigned) dp->addr) == sbusslot((unsigned) addr))) {
			dp->free = 0;
			return (dp->regs);
		}
		dp++;
	}
	return ((struct dmaga *) 0);
}

void
dma_free(regs)
struct dmaga *regs;
{
	register i;
	register struct dma_map *dp = dma_map;

	for (i = 0; i < ndma_map; i++) {
		if (!(dp->free) && dp->regs == regs) {
			dp->free = 1;
			break;
		}
		dp++;
	}
}

static int
dmaintr()
{
	return (0);
}

#endif	NDMA > 0
