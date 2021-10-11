#ifndef lint
static        char sccsid[] = "@(#)simdisk.c 1.1 92/07/30";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * Dummy driver for sparc simulated disks.
 *	uses systems calls via simulator to accomplish
 *	base level open, seek, read, and write operations
 */

int errno;

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/dk.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/dir.h>
#include <sys/user.h>
#include <sys/map.h>
#include <sys/vmmac.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <sys/kernel.h>
#include <sys/dkbad.h>
#include <sys/file.h>

#include <machine/pte.h>
#include <machine/psl.h>
#include <machine/mmu.h>
#include <machine/cpu.h>
#include <vm/page.h>

#include <sun/dklabel.h>
#include <sun/dkio.h>

#include <sundev/mbvar.h>

#include <sas_simdef.h>	/* Defintions for real devices used by simulator */

#ifdef IOMMU
#include <machine/iommu.h>
#endif IOMMU

int	nsimd = 0;

int	simdidentify();
int	simdattach();
int	simdintr();
int	simdstart();

#define	SIMDISK_OK	0
#define	SIMDISK_RD	1
#define	SIMDISK_WR	2
#define	SIMDISK_ERR	3

struct dev_ops simd_ops = {
	1,
	simdidentify,
	simdattach
};

int
simdidentify(name)
	char *name;
{
#if defined(SIMD_DEBUG)
	printf("simdidentify: <%s> ", name);
#endif
	if (strcmp(name, "simd")) {
#if defined(SIMD_DEBUG)
		printf("rejected\n\r", name);
#endif
		return 0;
	}
#if defined(SIMD_DEBUG)
	printf("recognized\n\r");
#endif
	nsimd ++;
	return 1;
}

union simcsr {
	struct csr {
		u_int	cnt	: 28; /* number of bytes to xfer/xferd */
		u_int	cmd	:  2; /* read or write */
		u_int	ie	:  1; /* interrupts enabled */
		u_int	go	:  1; /* do the transfer */
	}		s;
	unsigned	w;
};

struct simdregs {
	union simcsr	csr;
	unsigned	saddr;
	unsigned	part;
	unsigned	block;
};

#define	NPARTS		8
#define	UNIT(dev)	(((dev)&0x38) >> 3)
#define	PART(dev)	((dev)&7)
#define	DEV(u,p)	(((u)<<3)|(p))

struct simdisk {
	struct simdregs *regs;		/* mapped controller registers */
	int		sizes[NPARTS];	/* partition sizes */

	struct buf     *foq;		/* first on queue */
	struct buf     *loq;		/* last on queue */
	struct buf     *act;		/* current buf active */

	unsigned	addr;		/* base of dvma region */
	unsigned	offset;		/* total transferred so far */
}      *simdunits = 0;

int
simdattach(dev)
	struct dev_info *dev;
{
	struct simdisk *sd;
	static int unit = -1;
	register int part;
	int nreg, nint;

	unit ++;

#if defined(SIMD_DEBUG)
	printf("simdidentify: attaching simd%d\n", unit);
#endif

	if (!simdunits) {
		simdunits = (struct simdisk *)
			new_kmem_zalloc((u_int)(nsimd * sizeof (struct simdisk)),
					KMEM_SLEEP);
		if (!simdunits) {
			printf ("simd: no space for structures\n");
			return -1;
		}
	}
	dev->devi_unit = unit;
	sd = simdunits + unit;
	nreg = dev->devi_nreg;
	if (nreg != 1) {
		printf("simd: regs not specified correctly\n");
		return -1;
	}
/*
 * assumption: if we have different "simd" nodes, then
 * they talk via different registers to the simulator,
 * and hence get different "spindles".
 */
	sd->regs = (struct simdregs *)
		map_regs(dev->devi_reg->reg_addr,
			 dev->devi_reg->reg_size,
			 dev->devi_reg->reg_bustype);
	if (!sd->regs) {
		printf("simd: unable to map registers\n");
		return -1;
	}
	for (part=0; part<NPARTS; ++part) {
		sd->regs->part = part;
		sd->sizes[part] = sd->regs->block;
#if defined(SIMD_DEBUG)
		printf("simd%d: partition %d has %d blocks\n",
		       unit, part, sd->sizes[part]);
#endif
	}
	nint = dev->devi_nintr;
	if (nint != 1) {
		printf("simd: ints not specified correctly for unit %d\n",
		       unit);
		return -1;
	}
	addintr(dev->devi_intr->int_pri, simdintr, dev->devi_name, unit);
	report_dev(dev);
	return 0;
}

/*ARGSUSED*/
simdopen(dev, flag)
	register dev_t dev;
	register int flag;
{
	register int unit = UNIT(dev);
	register int part = PART(dev);

	if (unit >= nsimd) {
		printf("Illegal simulated disk unit (%d of %d).\n",
		       unit, nsimd);
		return (ENXIO);
	}

	if (part >= NPARTS) {
		printf("Illegal simulated disk partition (%d of %d).\n",
		       part, NPARTS);
		return (ENXIO);
	}

	return 0;
}

simdsize(dev)
	register dev_t dev;
{
	int	unit = UNIT(dev);
	int	part = PART(dev);
	int rv;

	if (!simdunits) {
#if defined(SIMD_DEBUG)
		printf("simdsize: simdunits not allocated\n");
#endif
		return 0;
	}
	if (unit >= nsimd) {
#if defined(SIMD_DEBUG)
		printf("simdsize: bad unit (%d of %d)\n",
		       unit, nsimd);
#endif
		return 0;
	}
	if (part >= NPARTS) {
#if defined(SIMD_DEBUG)
		printf("simdsize: bad part (%d of %d)\n",
		       part, NPARTS);
#endif
		return 0;
	}
	rv = simdunits[unit].sizes[part];
#if defined(SIMD_DEBUG)
	printf("simdsize: simd%d%c has %d blocks\n",
	       unit, part + 'a', rv);
#endif
	return rv;
}

simdstrategy(bp)
	register struct buf *bp;
{
	register int    unit;		/* unit number */
	register int    part;		/* partition number */
	struct simdisk *sd;
	int             s;		/* psr bits for splx */

	unit = UNIT(bp->b_dev);
	part = PART(bp->b_dev);
	sd = simdunits + unit;

	if (bp->b_bcount & (DEV_BSIZE - 1)) {
		printf("simd%d%c: non-block size i/o request\n", unit, part+'a');
		goto bad;
	}

	s = splhigh();		/* protect from simdintr */
	if (!sd->foq) {
		sd->foq = bp;
	} else {
		sd->loq->av_forw = bp;
	}
	bp->av_forw = NULL;
	sd->loq = bp;
	simdstart(sd);
	splx(s);		/* end of critical section */

	return;
bad:
	bp->b_flags |= B_ERROR;
	iodone(bp);
	return;
}

/*
 * simdstart: start a transfer.
 * must be at a high enough priority
 * that the sd->foq/sd->eoq structure
 * is protected from simdintr().
 * called from simdintr() and
 * from simdstrategy().
 */
simdstart(sd)
	register struct simdisk *sd;
{
	register struct buf *bp;	/* buf we want to transfer */
	register u_int  addr;		/* dvma cookie */

	for (bp = sd->foq; bp && !sd->act; bp = bp->av_forw) {
		addr = mb_mapalloc(dvmamap, bp, SBUS_MAP,
				   simdstart, (caddr_t)sd);
		if (!addr)
			break;
		bp->b_resid = bp->b_bcount;
		sd->act = bp;
		sd->addr = addr;
		sd->offset = 0;
		simdgo(sd);
	}
	sd->foq = bp;
	return 0;
}

/*
 * simdgo: sd->{act,addr,offset} refer to a transfer that needs
 * to be started. inform the simulator.
 */
simdgo(sd)
	struct simdisk *sd;
{
	int unit;
	int part;
	struct buf     *bp;
	unsigned        addr;
	union simcsr    t;
	unsigned        block;
	unsigned        count;

	bp = sd->act;
	if (!bp)
		return;
	addr = sd->addr + sd->offset + (u_int) DVMA;
	unit = minor(bp->b_dev);
	block = dkblock(bp) + (sd->offset >> DEV_BSHIFT);
	count = bp->b_resid - sd->offset;

	/* don't transfer across pages, thanks. */
	if (((addr & MMU_PAGEOFFSET) + count) > MMU_PAGESIZE)
		count = MMU_PAGESIZE - (addr & MMU_PAGEOFFSET);

	t.w = 0;
	t.s.ie = 1;
	t.s.cnt = count;
	if (bp->b_flags & B_READ)
		t.s.cmd = SIMDISK_RD;
	else
		t.s.cmd = SIMDISK_WR;
	t.s.go = 1;

	sd->regs->part = unit;
	sd->regs->block = block;
	sd->regs->saddr = addr;
	sd->regs->csr = t;
}

/*
 * simdintr: a transfer completed.
 * update the sd->* variables; if
 * there is more to do, poke simdgo();
 * otherwise, tell the kernel the transfer
 * has completed, and poke simdstart() to
 * get the next buffer on the list.
 */
simdintr()
{
	int             unit;
	struct simdisk *sd;
	struct buf     *bp;
	union simcsr    csr;
	int             count;
	int rv;

	rv = 0;
	for (unit=0; unit<nsimd; ++unit) {
		sd = simdunits + unit;
		bp = sd->act;
		if (!bp)
			continue; /* nobody active */
		csr = sd->regs->csr;
		if (csr.s.go)
			continue; /* still busy */

		if (csr.s.cmd == SIMDISK_ERR)
			bp->b_flags |= B_ERROR;

		count = csr.s.cnt;
		sd->offset += count;

		if ((sd->offset >= bp->b_bcount) ||
		    (bp->b_flags & B_ERROR)) {
			bp->b_resid = bp->b_bcount - sd->offset;
			mb_mapfree(dvmamap, &sd->addr);
			sd->act = 0;
			iodone(bp);
			simdstart(sd); /* do another */
		} else
			simdgo(sd); /* continue */
		rv = 1;
	}
	return rv;
}

simdcheck()
{
	if (nsimd > 0)
		return;
	panic("simd: missing devinfo node");
}
