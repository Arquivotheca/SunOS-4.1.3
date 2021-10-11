#if !defined(lint) && !defined(BOOTBLOCK)
static char sccsid[] = "@(#)boot_dk.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1989 Sun Microsystems, Inc.
 *
 * SAIO block (disk) device driver using OPENPROMs ROMvec calls
 */

#include <sys/types.h>
#include <stand/saio.h>
#include "boot/conf.h"

extern int romdev, rompclose(), rompopen(), nullprobe(), nullboot();
int blkstrategy();

struct boottab blkdriver = {
	"dk", nullprobe, nullboot, rompopen, rompclose, blkstrategy,
	0, 	/* b_desc  descriptive string */
	0,	/* b_devinfo memory allocation stuff */
};

/*ARGSUSED*/
int
blkstrategy(sip, rw)
register struct saioreq *sip;
register int rw;
{
	int i;
	int startblk = sip->si_bn;
	int xfersize = sip->si_cc/512;
	register struct binfo *bd;
	register daddr_t offset;

#ifdef DEBUG
	printf("blkstrategy(): %x %x %x ", sip->si_cc, sip->si_bn, sip->si_ma);

	if (sip->si_cc == 0)  {
		printf("blkstrategy: requested 0?\n");
		return (0);
	}
#endif DEBUG

#ifndef	sun4m
	if (prom_getversion() > 0) {
#endif	sun4m

		/* get file descriptor */
		bd = (struct binfo *)sip->si_devdata;
		romdev = bd->ihandle;
		offset = startblk * 512;
		if (prom_seek(romdev, 0, offset) < 0)
			return (0);
		if (rw == READ)
			i = (*romp->op2_read)(romdev, sip->si_ma, sip->si_cc);
		else
			i =(*romp->op2_write)(romdev, sip->si_ma, sip->si_cc);
#ifdef DEBUG
		printf("xfered %x bytes\n", i);

		if (i != sip->si_cc)
			printf("blkstrategy: read %d requested %d (BAD)\n",
			    i, sip->si_cc);
		if ((i == 0) || (i == -1))  {		/* error? */

			printf("blkstrategy(): PROM %s error (returned %d): ",
				rw == READ ? "read" : "write", i);
			printf("requested %x bytes, block number %x",
			    sip->si_cc, sip->si_bn);
			printf("PROM %s (blocked) failed\n",
				rw == READ ? "read" : "write");
		}
#endif	DEBUG
		return (i);
#ifndef sun4m
	}
	/* else */
	if (rw == READ)
		i = (*romp->v_read_blocks)(romdev, xfersize, startblk,
		    sip->si_ma);
	else
		i =(*romp->v_write_blocks)(romdev, xfersize, startblk,
		    sip->si_ma);
#ifdef DEBUG
	printf("xfered %x blocks\n", i);
#endif DEBUG
	return (i * 512);
#endif !sun4m
}
