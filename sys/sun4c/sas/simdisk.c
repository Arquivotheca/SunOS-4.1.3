#ifndef lint
static	char sccsid[] = "@(#)simdisk.c 1.1 92/07/30";
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

#include <sun/dklabel.h>
#include <sun/dkio.h>

#include <sundev/mbvar.h>

#include <sas_simdef.h>	/* Defintions for real devices used by simulator */
/*
 * XXX below should be defined as options to config
 */
struct simdisk {
	int	fdes;
	int	flag;
	int 	size;
	char	*name;
} simdunits[] = {
	-1, 0, SIMD_RSIZE, SIMD_RDEV,
	-1, 0, SIMD_SSIZE, SIMD_SDEV
};

#define MAXSIMDUNITS (sizeof (simdunits) / sizeof (simdunits[0]))

/*
 * Attach device (boot time).
 */
simdattach(md)
	register struct mb_device *md;
{ }

simdopen(dev, flag)
	register dev_t dev;
	register int flag;
{
	register int unit;
	register char *name;

	unit = minor(dev);

	if (unit >= MAXSIMDUNITS) {
		printf("Illegal simulated disk unit.\n");
		return (ENXIO);
	}

	simdunits[unit].flag = flag;
	name = simdunits[unit].name;

	/*
	 * We may be called to open a device more than once
	 * (e.g., mount -o remount re-opens the root)
	 * but since we always open the real file for read/write anyway
	 * we really don't need to obtain a new file descriptor.
	 */
	if ((simdunits[unit].fdes == -1) &&
	    (simdunits[unit].fdes = s_open(name, O_RDWR)) == -1 ) {
		 printf("Couldn't open %s\n", simdunits[unit].name);
		return (ENXIO);
	}

	return(0);
}

simdsize(dev)
	register dev_t dev;
{
	return (simdunits[minor(dev)].size);
}

simdstrategy(bp)
	register struct buf *bp;
{
	register daddr_t bn;		/* block number */
	register int unit;		/* unit number */
	register int fdes;		/* file descriptor */
	register int cnt;		/* completed transfer count */

	unit = minor(bp->b_dev);
	if (unit >= MAXSIMDUNITS) {
		printf("simd%d: simdstrategy: invalid unit\n", unit);
		bp->b_flags |= B_ERROR;
		iodone(bp);
		return;
	}

	bn = dkblock(bp);
	fdes = simdunits[unit].fdes;

	bp_mapin(bp);
	if (s_lseek(fdes, bn << DEV_BSHIFT, L_SET) == -1) {
		printf("simd%d: can't seek %s\n", unit, simdunits[unit].name);
		bp->b_flags |= B_ERROR;
	} else if (bp->b_bcount & (DEV_BSIZE - 1)) {
		printf("simd%d: non-block size i/o request\n", unit);
		bp->b_flags |= B_ERROR;
	} else if (bp->b_flags & B_READ) {
		if ((cnt=s_read(fdes, bp->b_un.b_addr, bp->b_bcount)) == -1) {
			printf("simd%d: error reading block %d\n", unit, bn);
			bp->b_flags |= B_ERROR;
		}
	} else {
		if ((cnt=s_write(fdes, bp->b_un.b_addr, bp->b_bcount)) == -1) {
			printf("simd%d: error writing block %d\n", unit, bn);
			bp->b_flags |= B_ERROR;
		}
	}
	bp_mapout(bp);
	if ((bp->b_flags & B_ERROR) == 0)
		bp->b_resid = bp->b_bcount - cnt;
	iodone(bp);
}
