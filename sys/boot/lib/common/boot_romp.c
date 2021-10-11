#if !defined(lint) && !defined(BOOTBLOCK)
static char	sccsid[] = "@(#)boot_romp.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1989 Sun Microsystems, Inc.
 *
 * Generic Tape Device (tape) saio support filter
 */

#include <sys/types.h>
#include <stand/saio.h>
#include "boot/conf.h"

int	romdev;

/*ARGSUSED*/
int
nullprobe(sip)
register struct saioreq *sip;
{
	return (0);
}


/*ARGSUSED*/
int
rompopen(sip)
register struct saioreq *sip;
{
	char	namebuf[50];
	register struct binfo *bd;
#ifndef	sun4m
	extern void formatdevname();

	if (prom_getversion() > 0) {
#endif	sun4m
		bd = (struct binfo *)sip->si_devdata;
		/* get path name from si_devdata */
		romdev = prom_open(bd->name);
		/* save file descriptor into si_devdata */
		bd->ihandle = romdev;
		return (romdev == 0);
#ifndef sun4m
	}
	formatdevname(namebuf, sip);

#ifdef DEBUG
	printf("rompopen(%s)\n", namebuf);
#endif DEBUG
	romdev = prom_open(namebuf);
	return (romdev == 0);
#endif !sun4m
}


/*ARGSUSED*/
int
rompclose(sip)
register struct saioreq *sip;
{
	register struct binfo *bd;

	if (prom_getversion() > 0) {
		/* get file descriptor */
		bd = (struct binfo *)sip->si_devdata;
		romdev = bd->ihandle;
	}
	return (prom_close(romdev));
}


/*ARGSUSED*/
int
nullboot(sip)
register struct saioreq *sip;
{
	return (0);
}


#ifndef	sun4m

/*
 * Format string of the form %c%c(%s,%s,%s) without using sprintf()
 * to keep bootblk nice and small.
 */

#define	HEX_BASE	16

void
formatdevname(p, sip)
	register char *p;
	register struct saioreq *sip;
{
	extern int sprintn();

	*p++ = sip->si_boottab->b_dev[0];
	*p++ = sip->si_boottab->b_dev[1];
	*p++ = '(';
	p += sprintn(p, sip->si_ctlr, HEX_BASE);
	*p++ = ',';
	p += sprintn(p, sip->si_unit, HEX_BASE);
	*p++ = ',';
	p += sprintn(p, sip->si_boff, HEX_BASE);
	*p++ = ')';
	*p++ = (char)0;
}
#endif	sun4m
