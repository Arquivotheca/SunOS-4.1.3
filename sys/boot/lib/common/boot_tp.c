#if !defined(lint) && !defined(BOOTBLOCK)
static char	sccsid[] = "@(#)boot_tp.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1989 Sun Microsystems, Inc.
 *
 * SAIO tape device driver using OPENPROMs ROMvec calls
 */

#include <sys/types.h>
#include <stand/saio.h>
#include "boot/conf.h"

extern int romdev, rompclose(), nullprobe(), nullboot();

int tapeopen(), tapestrategy();

struct boottab tapedriver = {
	"tp", nullprobe, nullboot, tapeopen, rompclose, tapestrategy,
	0, 	/* b_desc  descriptive string */
	0, 	/* b_devinfo memory allocation stuff */
};

int
tapeopen(sip)
register struct saioreq *sip;
{
	char namebuf[50];
	register struct binfo *bd;
#ifndef	sun4m
	extern void formatdevname();
#endif	sun4m

	if (prom_getversion() > 0) {
		bd = (struct binfo *)sip->si_devdata;
		/* get path name from si_devdata */
		romdev = prom_open(bd->name);
		/* save file descriptor into si_devdata */
		bd->ihandle = romdev;
		return (prom_seek(romdev, 0, 0));
	}
#ifndef sun4m
	formatdevname(namebuf, sip);
#ifdef DEBUG
	printf("tapeopen(%s)\n", namebuf);
#endif DEBUG
	if ((romdev = prom_open(namebuf)) < 0)
		return (-1);
#ifdef DEBUG
	printf("skipping %d files\n", sip->si_boff);
#endif DEBUG
	return (prom_seek(romdev, sip->si_boff, 0));
#endif !sun4m
}

/*ARGSUSED*/
int
tapestrategy(sip, rw)
register struct saioreq *sip;
register int rw;
{
	int i;
	register struct binfo *bd;

#ifdef DEBUG
	printf("tapestrategy(): %x %x %x ", sip->si_cc, sip->si_ma);
#endif DEBUG

	if (prom_getversion() > 0) {
		/* get file descriptor */
		bd = (struct binfo *)sip->si_devdata;
		romdev = bd->ihandle;
		if (rw == READ)
			i = (*romp->op2_read)(romdev, sip->si_ma, sip->si_cc);
		else
			i = (*romp->op2_write)(romdev, sip->si_ma, sip->si_cc);
#ifdef DEBUG
		printf("xfered %x bytes\n", i);
#endif DEBUG
		return (sip->si_cc);
	}
#ifndef sun4m
	/*
	 * XXX: so what does offset mean??
	 * offset from starting position?  offset from current position?
	 * I'll give it 0 for now
	 */
	if (rw == READ)
		i = (*romp->v_read_bytes)(romdev, sip->si_cc, 0, sip->si_ma);
	else
		i = (*romp->v_write_bytes)(romdev, sip->si_cc, 0, sip->si_ma);
#ifdef DEBUG
	printf("xfered %x bytes\n", i);
#endif DEBUG
	return (sip->si_cc);
#endif !sun4m
}
