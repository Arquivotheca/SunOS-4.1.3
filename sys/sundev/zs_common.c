#ifndef lint
static	char sccsid[] = "@(#)zs_common.c 1.4 90/09/18 SMI";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 *  Sun USART(s) driver - common code for all protocols
 */
#include "zs.h"
#if NZS > 0
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/syslog.h>

#include <machine/enable.h>
#include <machine/cpu.h>
#ifndef sun2
#include <machine/eeprom.h>
#endif sun2
#if defined(sun3) || defined(sun3x)
#include <machine/scb.h>
#endif sun3 || sun3x

#include <sun/fault.h>
#include <sun/consdev.h>
#ifdef	OPENPROMS
#include <sun/autoconf.h>
#endif	OPENPROMS

#include <sys/stream.h>
#include <sys/ttycom.h>
#include <sys/tty.h>
#include <sundev/mbvar.h>
#include <sundev/zsreg.h>
#include <sundev/zscom.h>

#ifdef	OPENPROMS
extern struct zscom *zscom;
struct zscom *zscurr, *zslast;
extern char	*zssoftCAR;
#else	OPENPROMS
extern struct zscom zscom[];
struct zscom *zscurr = &zscom[1];
struct zscom *zslast = &zscom[0];
extern char	zssoftCAR[];
#endif	OPENPROMS
extern int nzs;
struct zscom *zsNcurr;

#ifdef sun386
#ifdef SUN386
#define	ZSOFF 8
#endif SUN386
#ifdef AT386
#define	ZSOFF 2
#endif AT386
#else sun386
#define	ZSOFF 4
#endif

#define	ZREADA(n) \
		zszread((struct zscc_device *)((int)zs->zs_addr|ZSOFF), n)
#define	ZREADB(n) \
		zszread((struct zscc_device *)((int)zs->zs_addr&~ZSOFF), n)
#define	ZWRITEA(n, v) \
		zszwrite((struct zscc_device *)((int)zs->zs_addr|ZSOFF), n, v)
#define	ZWRITEB(n, v) \
		zszwrite((struct zscc_device *)((int)zs->zs_addr&~ZSOFF), n, v)

/*
 * Driver information for auto-configuration stuff.
 */
#ifndef OPENPROMS
#ifdef sun386
int	zshardware();
#endif
int	zsprobe(), zsattach(), zsintr();
struct	mb_device *zsinfo[NZS];
struct	mb_driver zsdriver = {
	zsprobe, 0, zsattach, 0, 0,
#ifdef sun386
	zshardware,
#else
	zsintr,
#endif
	2 * sizeof (struct zscc_device), "zs", zsinfo, 0, 0, 0,
};
#else	OPENPROMS

int	zsidentify(), zsintr();

#if defined(sun4m)
extern int zsintr_hi();
#endif

int	zsattach();
struct	dev_ops zs_ops = {
	1,
	zsidentify,
	zsattach,
};

struct dev_info	**zsinfo;	/* Index by zs->zs_unit */

#endif OPENPROMS

#ifdef	OPENPROMS
int
zsidentify(name)
	char *name;
{
	if (strcmp(name, "zs") == 0) {
		nzs += 2;	/* 2 zs units per actual device */
		return (1);
	} else
		return (0);
}


#else	OPENPROMS
/*ARGSUSED*/
zsprobe(reg, unit)
	caddr_t reg;
{
	register struct zscc_device *zsaddr = (struct zscc_device *)reg;
	struct zscom tmpzs, *zs = &tmpzs;
	short speed[2];
	register int c, loops;
	label_t jb;

	stop_mon_clock();
	/* get in sync with the chip */
	if ((c = peekc((char *)&zsaddr->zscc_control)) == -1) {
		start_mon_clock();
		return (0);
	}
	ZSDELAY(2);
	/*
	 * We see if it's a Z8530 by looking at register 15
	 * which always has two bits as zero.  If it's not a
	 * Z8530 then setting control to 15 will probably set
	 * those bits.  Hack, hack.
	 */
	if (pokec((char *)&zsaddr->zscc_control, 15)) {	/* set reg 15 */
		start_mon_clock();
		return (0);
	}

	ZSDELAY(2);
	if ((c = peekc((char *)&zsaddr->zscc_control)) == -1) {
		start_mon_clock();
		return (0);
	}
	ZSDELAY(2);
	if (c & 5) {
		start_mon_clock();
		return (0);
	}

	/*
	 * Well, that test wasn't strong enough for the damn UARTs
	 * on the video board in P2 memory, so here comes some more
	 * Anywhere in the following process, the non-existent video
	 * board may decide to give us a parity error, so we use nofault
	 * to catch any errors from here to the end of the probe routine
	 */
	zs->zs_addr = zsaddr;		/* for zszread/write */
	nofault = &jb;
	if (setjmp(nofault)) {
		start_mon_clock();
		/* error occurred */
		goto error;
	}

	/*
	 * We can't trust the drain bit in the uart cause we don't know
	 * that the uart is really there.
	 * We need this because trashing the speeds below causes garbage
	 * to be sent.
	 */
	loops = 0;
	while ((ZREADA(1) & ZSRR1_ALL_SENT) == 0 ||
	    (ZREADB(1) & ZSRR1_ALL_SENT) == 0 ||
	    (ZREADA(0) & ZSRR0_TX_READY) == 0 ||
	    (ZREADB(0) & ZSRR0_TX_READY) == 0) {
		DELAY(1000);
		if (loops++ > 500)
			break;
	}
	/* must preserve speeds for monitor / console */
	speed[0] = ZREADA(12);
	speed[0] |= ZREADA(13) << 8;
	speed[1] = ZREADB(12);
	speed[1] |= ZREADB(13) << 8;
	ZWRITEA(12, 17);
	ZWRITEA(13, 23);
	ZWRITEB(12, 29);
	ZWRITEB(13, 37);
	if ((c = ZREADA(12)) != 17) {
		start_mon_clock();
		goto error;
	}
	if ((c = ZREADA(13)) != 23) {
		start_mon_clock();
		goto error;
	}
	if ((c = ZREADB(12)) != 29) {
		start_mon_clock();
		goto error;
	}
	if ((c = ZREADB(13)) != 37) {
		start_mon_clock();
		goto error;
	}
	/* restore original speeds */
	ZWRITEA(12, speed[0]);
	ZWRITEA(13, speed[0] >> 8);
	ZWRITEB(12, speed[1]);
	ZWRITEB(13, speed[1] >> 8);
	nofault = 0;
	start_mon_clock();
	return (2 * sizeof (struct zscc_device));
error:
	nofault = 0;
	return (0);
}
#endif	OPENPROMS

#if defined(sun3) || defined(sun3x)
/*
 * Base vector numbers for SCC chips
 * Each SCC chip requires 8 contiguous even or odd vectors,
 * on a multiple of 16 boundary
 * E.G., nnnnxxxn where nnnn000n is the base value
 */
short zsvecbase[] = {
	144,		/* zs0 - 1001xxx0 */
	145,		/* zs1 - 1001xxx1 */
};
#define	NZSVEC	(sizeof zsvecbase/sizeof zsvecbase[0])
#endif sun3 || sun3x

#ifdef	OPENPROMS

zsattach(dev)
	register struct dev_info *dev;
{
	static int curzs = 0;
	register struct zscom *zs;
	register struct zsops *zso;
	register int i, j;
	short speed[2];
	int loops;
#ifndef	sun4c
	short vector = 0;
#endif
	struct zscc_device *tmpzs;	/* for mapping purposes */

	dev->devi_unit = curzs;
	if (zscom == NULL) {
		zscom = (struct zscom *)new_kmem_zalloc(
			(u_int) (nzs * sizeof (struct zscom)), KMEM_SLEEP);
		zssoftCAR = (char *)new_kmem_zalloc(
			(u_int) (nzs * sizeof (char)), KMEM_SLEEP);
		zsinfo = (struct dev_info **)new_kmem_zalloc(
			(u_int)(nzs * sizeof (struct dev_info *)), KMEM_SLEEP);
		zscurr = &zscom[1];
		zslast = &zscom[0];
		if (zscom == NULL || zssoftCAR == NULL || zsinfo == NULL) {
			printf("zs: no space for structures\n");
			return (-1);
		}
	}


	zs = &zscom[dev->devi_unit*2];
	stop_mon_clock();	/* stop monitor's polling interrupt */

	/* map in the device registers */
	if (dev->devi_nreg == 1) {
		zs->zs_addr = tmpzs = (struct zscc_device *)
			map_regs(dev->devi_reg->reg_addr,
				dev->devi_reg->reg_size,
				dev->devi_reg->reg_bustype);
	} else {
		printf("zs%d: warning: bad register specification\n", curzs);
		return (-1);
	}

	loops = 0;
	while ((ZREADA(1) & ZSRR1_ALL_SENT) == 0 ||
	    (ZREADB(1) & ZSRR1_ALL_SENT) == 0 ||
	    (ZREADA(0) & ZSRR0_TX_READY) == 0 ||
	    (ZREADB(0) & ZSRR0_TX_READY) == 0) {
		DELAY(1000);
		if (loops++ > 500)
			break;
	}
	/* must preserve speeds over reset for monitor */
	speed[0] = ZREADA(12);
	speed[0] |= ZREADA(13) << 8;
	speed[1] = ZREADB(12);
	speed[1] |= ZREADB(13) << 8;
	ZWRITE(9, ZSWR9_RESET_WORLD); DELAY(10);
	zs->zs_wreg[9] = 0;
	for (i = 0; i < 2; i++) {
		if (i == 0) {		/* port A */
			zs->zs_addr = (struct zscc_device *)
					((int)tmpzs | ZSOFF);
		} else {		/* port B */
			zs++;
			zs->zs_addr = (struct zscc_device *)
					((int)tmpzs &~ ZSOFF);
			zscurr = zs;
		}
		zs->zs_unit = dev->devi_unit * 2 + i;
		zsinfo[zs->zs_unit] = dev;
		zssoftCAR[zs->zs_unit] =
		    getprop(dev->devi_nodeid,
			i ? "port-b-ignore-cd" : "port-a-ignore-cd", 0);
		for (j=0; zs_proto[j]; j++) {
			zso = zs_proto[j];
			(*zso->zsop_attach)(zs, speed[i]);
		}
	}
	ZWRITE(9, ZSWR9_MASTER_IE + ZSWR9_VECTOR_INCL_STAT);
#ifndef	sun4c
	if (vector)
		ZWRITE(2, vector);
#endif
	DELAY(4000);
	start_mon_clock();	/* re-enable monitor's polling interrupt */
	zslast = zs;

	/*
	 * Two levels of interrupt - chip interrupts at level12
	 * (fielded in assembler and then seen below as misnomer
	 *  'zslevel6intr') and then, using software level6 interrupt
	 * seen in zsintr below. NOTE: except in sun4m, the high
	 * priority interupt is wired down, so we need not add it.
	 * the low priority interrupt is just the driver talking
	 * to itsself, so we add it here.
	 *
	 * also, on sun4m, the devinfo tree knows best.
	 */
	if (dev->devi_nintr) {
#ifndef sun4m
/*
 * zslevel12 is already attached at SPARC level 12; bitch
 * if the device wants it elsewhere.
 */
		if (dev->devi_intr->int_pri != 12) {
			printf("zs%d: priority %d\n",
				dev->devi_unit, dev->devi_intr->int_pri);
			panic("bad zs priority");
		}
#else	sun4m
		addintr(dev->devi_intr->int_pri,
			zsintr_hi, dev->devi_name, curzs);
#endif	sun4m
		addintr(
#ifdef INTLEVEL_SOFT
/* tell autoconf we are interested in SOFT ints at this level */
			INTLEVEL_SOFT +
#endif
			ZS_SOFT_INT, zsintr, dev->devi_name, curzs);
	}

	report_dev(dev);
	curzs++;
	return (0);
}

#else	OPENPROMS

zsattach(md)
	register struct mb_device *md;
{
	register struct zscom *zs = &zscom[md->md_unit*2];
	register struct zsops *zso;
	register int i, j;
	short speed[2];
	int loops;
	short vector = 0;

#if defined(sun3) || defined(sun3x)
	/*
	 * Install the 8 vectors for this SCC chip
	 */
#ifdef sun3
	if ((cpu != CPU_SUN3_50) && (cpu != CPU_SUN3_60) &&
	    (cpu != CPU_SUN3_E)) {
#endif sun3
#ifdef sun3x
	if (cpu == CPU_SUN3X_470) {
#endif sun3x
		extern int (*zsvectab[NZS][8])();
		int (**p)(), (**q)();

		if (md->md_unit >= NZSVEC)
			panic("zsattach: too many zs units");
		vector = zsvecbase[md->md_unit];
		p = &scb.scb_user[vector - VEC_MIN];
		q = &zsvectab[md->md_unit][0];
		for (i = 0; i < 8; i++) {
			*p = *q++;
			p += 2;
		}
	}
#endif sun3 || sun3x
	stop_mon_clock();	/* stop monitor's polling interrupt */
	zs->zs_addr = (struct zscc_device *)md->md_addr;
	loops = 0;
	while ((ZREADA(1) & ZSRR1_ALL_SENT) == 0 ||
	    (ZREADB(1) & ZSRR1_ALL_SENT) == 0 ||
	    (ZREADA(0) & ZSRR0_TX_READY) == 0 ||
	    (ZREADB(0) & ZSRR0_TX_READY) == 0) {
		DELAY(1000);
		if (loops++ > 500)
			break;
	}
	/* must preserve speeds over reset for monitor */
	speed[0] = ZREADA(12);
	speed[0] |= ZREADA(13) << 8;
	speed[1] = ZREADB(12);
	speed[1] |= ZREADB(13) << 8;
	ZWRITE(9, ZSWR9_RESET_WORLD); DELAY(10);
	zs->zs_wreg[9] = 0;
	for (i = 0; i < 2; i++) {
		if (i == 0) {		/* port A */
			zs->zs_addr = (struct zscc_device *)
				((int)md->md_addr | ZSOFF);
		} else {		/* port B */
			zs++;
			zs->zs_addr = (struct zscc_device *)
					((int)md->md_addr &~ ZSOFF);
			zscurr = zs;
		}
		zs->zs_unit = md->md_unit * 2 + i;
		zssoftCAR[zs->zs_unit] = md->md_flags & (1 << i);
		for (j=0; zs_proto[j]; j++) {
			zso = zs_proto[j];
			(*zso->zsop_attach)(zs, speed[i]);
		}
	}
	ZWRITE(9, ZSWR9_MASTER_IE + ZSWR9_VECTOR_INCL_STAT);
	if (vector)
		ZWRITE(2, vector);
	DELAY(4000);
	start_mon_clock();	/* re-enable monitor's polling interrupt */
	zslast = zs;
#ifndef sun386
	if (md->md_intpri != 3) {
		printf("zs%d: priority %d\n", md->md_unit, md->md_intpri);
		panic("bad zs priority");
	}
#endif !sun386

}

#endif	OPENPROMS

/*
 *
 * Handle Hardware level 6 interrupts These interrupts are locked out only
 * by splzs or spl7, not by spl6, so this routine may not use UNIX
 * facilities such as wakeup which depend on being able to disable
 * interrupts with spls.  All communication with the rest of the world is
 * done through the zscom structure and the use of level 3 software
 * interrupts.
 *
 * This routine is only called when the vector indicated by the most
 * recently interrupting SCC is a "special receive" interrupt.  This
 * vector is used BOTH for special receive interrupts and to indicate NO
 * interrupt pending.  In the no interrupt pending case we must poll the
 * other SCCs to find the interrupter.  Low level assembler code
 * dispatches the other vectors using the zs_vec array.  This assembler
 * routine is also the code which actually clears the interrupt; the argzs
 * argument is a value/return argument which changes when a different SCC
 * interrupts.
 *
 * Note that the '6' in zslevel6intr is a misnomer- actually, on sparc
 * systems that actual interrupt level is level 10, or maybe level 12,
 * or possibly something different ... and, value/return parameters
 * don't work, and sun4m can tell us which SCCs are requesting. All
 * in all, the whole high priority distrubution scheme needs to be
 * revisited in our copious spare time.
 */
zslevel6intr(argzs)
	struct zscom *argzs;		/* NOTE: value/return argument!! */
{
	register struct zscom *zs;
	register short iinf, unit;

	zs = zscurr;		/* always channel B */
	unit = 0;
	for (;;) {
		if (zs->zs_addr && ZREADA(3))
			break;
		zs += 2;		/* always channel B */
		if (zs > zslast)
			zs = &zscom[1];
		if (++unit >= (nzs>>1))	/* divide by 2 because nzs==NZS*2 */
			return;
	}
	zscurr = zs;
	iinf = ZREAD(2);	/* get interrupt vector & status */
	if (iinf & 8)
		zs = zscurr - 1;	/* channel A */
	else
		zs = zscurr;		/* channel B */
	switch (iinf & 6) {
	case 0:		/* xmit buffer empty */
		(*zs->zs_ops->zsop_txint)(zs);
		break;

	case 2:		/* external/status change */
		(*zs->zs_ops->zsop_xsint)(zs);
		break;

	case 4:		/* receive char available */
		(*zs->zs_ops->zsop_rxint)(zs);
		break;

	case 6:		/* special receive condition or no interrupt */
		(*zs->zs_ops->zsop_srint)(zs);
		break;
	}
	zsNcurr = zs;		/* for when value/return can't be made to work */
	argzs = zs;
#ifdef lint
	argzs = argzs;
#endif lint
}

#ifdef sun386
zshardware()
{
	register struct zscom *zs;
	register short iinf, unit;
	register short stat;

	for (;;) {
		zs = zscurr;		/* always channel B */
		unit = 0;
		for (;;) {
			if (zs->zs_addr && (stat = ZREADA(3)))
				break;
			zs += 2;	/* always channel B */
			if (zs > zslast)
				zs = &zscom[1];
			if (++unit >= NZS)
				return;
		}
		zscurr = zs;
		iinf = ZREAD(2);	/* get interrupt vector & status */
		if (iinf & 8) {
			zs = zscurr - 1;	/* channel A */
			stat >>= 3;
		} else
			zs = zscurr;		/* channel B */

		do {
			if (!(stat & 7)) {	/* special receive */
				(*zs->zs_ops->zsop_srint)(zs);
				stat = 0;
			}
			if (stat & 2) {		/* transmit buffer empty */
				(*zs->zs_ops->zsop_txint)(zs);
				stat &= ~2;
			}
			if (stat & 4) {		/* receive char available */
				while (ZREAD(0) & ZSRR0_RX_READY)
					(*zs->zs_ops->zsop_rxint)(zs);
				stat &= ~4;
			}
			if (stat & 1) {		/* external/status info */
				(*zs->zs_ops->zsop_xsint)(zs);
				stat &= ~1;
			}

		} while (stat);
		zs->zs_addr->zscc_control = ZSWR0_CLR_INTR;
	}
}
#endif

/*
 * Install a new ops vector into low level vector routine addresses
 */
zsopinit(zs, zso)
	register struct zscom *zs;
	register struct zsops *zso;
{

	zs->zs_vec[0] = zso->zsop_txint;
	zs->zs_vec[1] = zso->zsop_xsint;
	zs->zs_vec[2] = zso->zsop_rxint;

	switch (cpu) {
#ifdef sun3
	case CPU_SUN3_160:
	case CPU_SUN3_260:
	case CPU_SUN3_110:
		/* vectored interrupts */
		zs->zs_vec[3] = zso->zsop_srint;
		break;
#endif sun3
#ifdef sun3x
	case CPU_SUN3X_470:
		/* vectored interrupts */
		zs->zs_vec[3] = zso->zsop_srint;
		break;
#endif sun3x
	default:
		/* non-vectored Sun-2, Sun-4 and Sun-3 50 (Model 25) */
		/* sun3x/80, sun4c/60, sun4m */
		zs->zs_vec[3] = zslevel6intr;
		break;
	}
	zs->zs_ops = zso;
}

/*
 * Handle a level 3 interrupt
 * This is the routine found by autoconf in the driver structure
 * except on openboot machines, where it is known only by the
 * driver and presented manually to addintr.
 */
zsintr()
{
	register struct zscom *zs;

	if (clrzssoft()) {
		zssoftpend = 0;
		for (zs = &zscom[0]; zs <= zslast; zs++) {
			if (zs->zs_flags & ZS_NEEDSOFT) {
				zs->zs_flags &=~ ZS_NEEDSOFT;
				(*zs->zs_ops->zsop_softint)(zs);
			}
		}
		return (1);
	}
	return (0);
}

/*
 * The "null" zs protocol
 * Called before the others to initialize things
 * and prevent interrupts on unused devices
 */
int	zsnull_attach(), zsnull_intr(), zsnull_softint();

struct zsops zsops_null = {
	zsnull_attach,
	zsnull_intr,
	zsnull_intr,
	zsnull_intr,
	zsnull_intr,
	zsnull_softint,
};

zsnull_attach(zs, speed)
	register struct zscom *zs;
{
	int no_rtsdtr = 0;

	/* make sure ops prt is valid */
	zsopinit(zs, &zsops_null);

#ifndef sun2
	/* check prom as to whether RTS and DTR should be raised at init */
#ifdef	OPENPROMS
	if ((zs->zs_unit == 0 &&
	    getprop(zsinfo[0]->devi_nodeid, "port-a-rts-dtr-off", 0)) ||
	(zs->zs_unit == 1 &&
	    getprop(zsinfo[1]->devi_nodeid, "port-b-rts-dtr-off", 0)))
		no_rtsdtr = 1;
#else	OPENPROMS
	if ((zs->zs_unit == 0 &&
	    EEPROM->ee_diag.eed_ttya_def.eet_rtsdtr == NO_RTSDTR) ||
	(zs->zs_unit == 1 &&
	    EEPROM->ee_diag.eed_ttyb_def.eet_rtsdtr == NO_RTSDTR))
		no_rtsdtr = 1;
#endif	OPENPROMS
#endif sun2
	/*
	 * Set up the default asynch modes
	 * so the monitor will still work
	 */
	ZWRITE(4, ZSWR4_PARITY_EVEN + ZSWR4_1_STOP + ZSWR4_X16_CLK);
	ZWRITE(3, ZSWR3_RX_8);
	ZWRITE(11, ZSWR11_TXCLK_BAUD + ZSWR11_RXCLK_BAUD);
	ZWRITE(12, speed);
	ZWRITE(13, speed >> 8);
	ZWRITE(14, ZSWR14_BAUD_FROM_PCLK);
	ZWRITE(3, ZSWR3_RX_8 + ZSWR3_RX_ENABLE);
	if (no_rtsdtr)
		ZWRITE(5, ZSWR5_TX_ENABLE + ZSWR5_TX_8);
	else
		ZWRITE(5, ZSWR5_TX_ENABLE + ZSWR5_TX_8 + ZSWR5_RTS + ZSWR5_DTR);
	ZWRITE(14, ZSWR14_BAUD_ENA + ZSWR14_BAUD_FROM_PCLK);

	/*
	 * The following hack is because the sync lines on the kb/ms scc
	 * are not connected.  Noise creates transitions on the sync lines
	 * which then flood the system with interrupts, unless the
	 * Sync/Hunt IE is disabled.
	 *
	 * Is it really cool to wire down here that units 2 and 3 are
	 * the keyboard and mouse units?
	 */
	if ((zs->zs_unit == 2) || (zs->zs_unit == 3))
		ZWRITE(15, ZSR15_BREAK + ZSR15_TX_UNDER + ZSR15_CTS + ZSR15_CD);

	zs->zs_addr->zscc_control = ZSWR0_RESET_ERRORS + ZSWR0_RESET_STATUS;
	/* XXX we assume there is enough time here for recovery */
}

zsnull_intr(zs)
	register struct zscom *zs;
{
	register struct zscc_device *zsaddr = zs->zs_addr;
	register short c;

	zsaddr->zscc_control = ZSWR0_RESET_TXINT;
	ZSDELAY(2);
	zsaddr->zscc_control = ZSWR0_RESET_STATUS;
	ZSDELAY(2);
	c = zsaddr->zscc_data;
#ifdef lint
	c = c;
#endif lint
	ZSDELAY(2);
	zsaddr->zscc_control = ZSWR0_RESET_ERRORS;
	/* XXX we assume there is enough time here for recovery */
}

zsnull_softint(zs)
	register struct zscom *zs;
{
	log(LOG_WARNING, "zs%d: unexpected soft int\n", zs->zs_unit);
}
#endif NZS > 0
