#ifndef lint
static	char sccsid[] = "@(#)simdisk.c 1.1 92/07/30";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 * Dummy driver for sparc simulated disks.
 *	uses systems calls via simulator to accomplish
 *	base level open, seek, read, and write operations
 */

#include <stand/saio.h>
#include <stand/param.h>
#include <sys/file.h>
#include <sas_simdef.h>

struct simdisk {
	int	fdes;
	int 	size;
	char	*name;
} simdunits[] = {
	-1, SIMD_RSIZE, SIMD_RDEV,
	-1, SIMD_SSIZE, SIMD_SDEV,
};

#define MAXSIMDUNITS (sizeof (simdunits) / sizeof (struct simdisk))

simdopen(sip)
	register struct saioreq *sip;
{
	register int unit = sip->si_unit;

	if (unit >= MAXSIMDUNITS) {
		printf("Illegal simulated disk unit.\n");
		return (-1);
	}
	/*
	 * We may be called to open a device more than once
	 * (e.g., mount -o remount re-opens the root)
	 * but since we always open the real file for read/write anyway
	 * we really don't need to obtain a new file descriptor.
	 */
	if ((simdunits[unit].fdes == -1) &&
	    (simdunits[unit].fdes = s_open(simdunits[unit].name, O_RDWR)) == -1) {
		printf("Couldn't open %s\n", simdunits[unit].name);
		return (-1);
	}
	return(0);
}

simdstrategy(sip, rw)
	register struct saioreq *sip;
	register int rw;
{
	register int unit;		/* unit number */
	register int fdes;		/* file descriptor */
	register int cnt;		/* completed transfer count */

	unit = minor(sip->si_unit);
	if (unit >= MAXSIMDUNITS) {
		printf("simd%d: simdstrategy: invalid unit\n", unit);
		return(-1);
	}

	fdes = simdunits[unit].fdes;

	if (s_lseek(fdes, sip->si_bn << DEV_BSHIFT, L_SET) == -1) {
		printf("simd%d: can't seek %s\n", unit, simdunits[unit].name);
		return(-1);
	} else if (sip->si_cc & (DEV_BSIZE - 1)) {
		printf("simd%d: non-block size i/o request\n", unit);
		return(-1);
	} else if (rw != WRITE) {
		if ((cnt=s_read(fdes, sip->si_ma, sip->si_cc)) == -1) {
			printf("simd%d: error reading block %d\n",
			    unit, sip->si_bn);
			return(-1);
		}
	} else {
		if ((cnt=s_write(fdes, sip->si_ma, sip->si_cc)) == -1) {
			printf("simd%d: error writing block %d\n",
			    unit, sip->si_bn);
			return(-1);
		}
	}
	return(sip->si_cc - cnt);
}
