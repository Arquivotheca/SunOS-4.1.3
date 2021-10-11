#ident	"@(#)ledma.c 1.1 92/07/30 SMI"

/*
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 */

#include "le.h"
#if	NLE > 0

/*
 * DMA2 "ledma" driver.
 *
 * This driver identifies "ledma", maps in the DMA2 E-CSR register,
 * attaches the child node "le", and calls the le driver routine
 * leopsadd() to add an entry to the le driver ops linked list
 * indicating the DMA2-specific init() and intr() routines
 * to be called by the le driver from leinit() and leintr().
 */

#include <sys/param.h>
#include <sys/buf.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/debug.h>
#include <sundev/mbvar.h>
#include <sun/autoconf.h>
#include <machine/mmu.h>
#include <sys/socket.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <sunif/if_lereg.h>
#include <sunif/if_levar.h>

/*
 * DMA2 E-CSR bit definitions.
 * Refer to the Campus II DMA2 Chip Specification for details.
 */
#define	E_INT_PEND	(1 << 0)	/* e_irq_ is active */
#define	E_ERR_PEND	(1 << 1)	/* memory time-out, protection, */
					/* or parity */
#define	E_DRAINING	(3 << 2)	/* both bits set when e-cache is */
					/* draining */
#define	E_INT_EN	(1 << 4)	/* enable intr on E_INT_PEND or */
					/* E_ERR_PEND */
#define	E_INVALIDATE	(1 << 5)	/* invalidate e-cache */
#define	E_SLAVE_ERR	(1 << 6)	/* slave access size error */
#define	E_RESET		(1 << 7)	/* reset lance interface & */
					/* invalidate e-cache */
#define	E_DRAIN		(1 << 10)	/* drain e-cache */
#define	E_DSBL_WR_DRN	(1 << 11)	/* disable e-cache drain on */
					/* lance descr writes */
#define	E_DSBL_RD_DRN	(1 << 12)	/* disable e-cache drain on */
					/* slave reads */
#define	E_ILACC		(1 << 15)	/* modify dma cycle for ilacc */
					/* (untested) */
#define	E_DSBL_BUF_WR	(1 << 16)	/* disable slave write buffer to */
					/* lance */
#define	E_DSBL_WR_INVAL	(1 << 17)	/* disable slave write auto */
					/* e-cache invalidate */
#define	E_BURST_SIZE	(3 << 18)	/* sbus burst sizes mask */
#define	E_ALE_AS	(1 << 20)	/* define pin 35 active high/low */
#define	E_LOOP_TEST	(1 << 21)	/* enable external ethernet loopback */
#define	E_TP_AUI	(1 << 22)	/* select TP or AUI */
#define	E_DEV_ID	(0xf << 28)	/* device id mask */

#define	E_DMA2_ID	0xa0000000	/* DMA2 E-CSR device id */

#define	E_BURST_MASK	0x000C0000	/* dma2 e-csr burst size mask */

/*
 * DMA2 E-CSR SBus burst sizes
 */
#define	E_BURST16	(0 << 18)	/* 16 byte SBus bursts */
#define	E_BURST32	(1 << 18)	/* 32 byte SBus bursts */


static int ledmaidentify(), ledmaattach(), ledmainit(), ledmaintr();
struct	dev_ops ledma_ops = {
	1,
	ledmaidentify,
	ledmaattach
};

static int
ledmaidentify(name)
char *name;
{
	if (strcmp(name, "ledma") == 0)
		return (1);
	else
		return (0);
}

int	ledmabufsize = 128 * 1024;	/* 128K ledma buffer size */

static int
ledmaattach(dev)
register struct dev_info *dev;
{
	static	int	unit = 0;
	u_int	*csr;
	caddr_t	membase = NULL;
	caddr_t	iobase = NULL;
	struct	leops	*lop;

	dev->devi_unit = unit++;

	if ((dev->devi_nreg > 1) ||
		(dev->devi_reg->reg_size < sizeof (u_int))) {
		printf("ledma%d: bad regs specification\n", dev->devi_unit);
		return (-1);
	}

	/*
	 * Map in the E-CSR control register.
	 */
	csr = (u_int *) map_regs(dev->devi_reg->reg_addr, sizeof (u_int),
		dev->devi_reg->reg_bustype);
	if (csr == NULL) {
		printf("ledma%d: unable to map registers\n", dev->devi_unit);
		return (-1);
	}

	/*
	 * Sanity check.
	 */
	if ((*csr & E_DEV_ID) != E_DMA2_ID) {
		printf("ledma%d: invalid DMA2 E-CSR format: 0x%x\n",
			dev->devi_unit, *csr);
		return (-1);
	}

	dev->devi_data = (caddr_t) csr;

#ifdef	IOMMU
	/*
	 * For platforms with an IOMMU, such as those based on Sun-4M,
	 * we allocate some kernel memory, statically IO map it, and
	 * treat it as a Programmed I/O device (copy to/from) rather
	 * than dynamically IO mapping and unmapping each buffer to
	 * transmit and receive.  This may change if the cost of
	 * IO mapping gets cheaper but unfortunately it's the best
	 * tradeoff for now.
	 *
	 * 'membase' is the CPU-addressable memory base
	 * 'iobase' is the device-addressable memory base
	 */
	if ((membase = kmem_alloc((u_int) ledmabufsize)) == NULL) {
		printf("ledma%d: kmem_alloc(%d) failed\n", dev->devi_unit,
			ledmabufsize);
		return (-1);
	}
	/* NOSTRICT */
	if ((iobase = (caddr_t) mb_nbmapalloc(dvmamap, membase,
		ledmabufsize, MDR_BIGSBUS, (int(*)())0, (caddr_t) 0)) == NULL) {
		printf("ledma%d: mb_mapalloc failed\n", dev->devi_unit);
		kmem_free(membase, (u_int) ledmabufsize);
		return (-1);
	}
#endif	IOMMU

	report_dev(dev);
	attach_devs(dev);	/* attach child "le" node */

	/*
	 * Add entry to le driver ops list
	 */
	if ((lop = (struct leops *) kmem_alloc(sizeof (struct leops)))
		== NULL) {
		printf("lebuf%d:  kmem_alloc failed!\n", dev->devi_unit);
		return (-1);
	}
	lop->lo_dev = dev->devi_slaves;
	lop->lo_flags = LO_PIO | LO_IOADDR;
	lop->lo_membase = membase;
	lop->lo_iobase = iobase;
	lop->lo_size = ledmabufsize;
	lop->lo_init = ledmainit;
	lop->lo_intr = ledmaintr;
	lop->lo_arg = (caddr_t) dev;
	leopsadd(lop);

	return (0);
}

static	int	le_last_tp_aui;
/*
 * "Exported" by address and called by the le driver.
 */
static
ledmainit(dev, tp_aui, init)
struct dev_info *dev;
int	tp_aui;
int	init;
{
	u_int	*csr = (u_int *) dev->devi_data;
	struct	dev_info	*dip;
	int	burst;
	u_int	tcsr;

	/*
	 * A nonzero tp_aui argument means Twisted Pair.
	 * Zero means AUI.
	 */
	if (tp_aui)
		tp_aui = E_TP_AUI;


	/*
	 * Hard reset.
	 * Readback flushes any system write buffers.
	 */
	if (init) {
		*csr = E_RESET;

		if (*csr)	/* readback */
			;

		if (tp_aui)
			/*
			 * AT&T 7213 requires us to wait for 20ms during the
			 * switch between TP and AUI.
			 */
			DELAY(20000);
		else
			DELAY(100);

		/*
		 * Determine proper SBus burst size to use.
		 */
		for (dip = dev->devi_parent; dip != top_devinfo;
			dip = dip->devi_parent)
			if (strcmp(dip->devi_name, "sbus") == 0)
				break;
		if (dip == top_devinfo) {
			printf("ledmainit:  parent sbus node not found!\n");
			return;
		}
		burst = (getprop(dip->devi_nodeid, "burst-sizes", 0) & 32) ?
			E_BURST32 : E_BURST16;

		/*
		 * Initialize E_CSR.
		 */
		*csr = E_INT_EN | tp_aui | E_INVALIDATE | E_DSBL_RD_DRN |
			E_DSBL_WR_INVAL | burst;

		if (*csr)	/* readback */
			;

		DELAY(20000);
		le_last_tp_aui = tp_aui;
	} else
		/*
		 * Avoid to delay 20ms unless we actually need to
		 * switch the TP/AUI.  Here we assume that the driver has
		 * already called ledmainit() at least once with the
		 * init being 1.
		 */
		if (tp_aui != le_last_tp_aui) {

			/*
			 * Just set the TP/AUI bits.
			 */
			tcsr = *csr & ~E_TP_AUI;
			tcsr |= tp_aui;
			*csr = tcsr;

			if (*csr)	/* readback */
				;

			DELAY(20000);
			le_last_tp_aui = tp_aui;
		}
}

/*
 * "Exported" by address and called by the le driver.
 * Return 1 if "serviced", 0 otherwise.
 */
static
ledmaintr(dev)
struct dev_info *dev;
{
	u_int	*csr = (u_int *) dev->devi_data;

	if (*csr & (E_ERR_PEND | E_SLAVE_ERR)) {
		if (*csr & E_ERR_PEND)
			printf("DMA:  memory time out or protection error!\n");
		if (*csr & E_SLAVE_ERR)
			printf("DMA:  LANCE register slave access error!\n");
		return (1);
	}

	return (0);
}

#endif	NLE > 0
