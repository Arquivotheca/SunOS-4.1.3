#ifndef lint
static	char sccsid[] = "@(#)vp.c 1.1 92/07/30 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * Versatec matrix printer/plotter
 * dma interface driver for Ikon 10071-5 Multibus/Versatec interface
 */
#include "vp.h"
#include <sys/param.h>
#include <sys/dir.h>
#include <sys/user.h>
#include <sys/buf.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/map.h>
#include <sys/ioctl.h>
#include <sys/vcmd.h>
#include <sys/uio.h>

#include <machine/psl.h>
#include <machine/mmu.h>
#include <sundev/vpreg.h>
#include <sundev/mbvar.h>

#define	VPPRI	(PZERO-1)

struct vp_softc {
	int	sc_state;
	struct buf *sc_bp;
	int	sc_mbinfo;
} vp_softc[NVP];

#define	VPSC_BUSY	0400000
/* sc_state bits - passed in VGETSTATE and VSETSTATE ioctls */
#define	VPSC_MODE	0000700
#define	VPSC_SPP	0000400
#define	VPSC_PLOT	0000200
#define	VPSC_PRINT	0000100
#define	VPSC_CMNDS	0000076
#define	VPSC_OPEN	0000001

#define	VPUNIT(dev)	(minor(dev))

struct	buf rvpbuf[NVP];

int vpprobe(), vpintr();

struct	mb_device *vpdinfo[NVP];
struct mb_driver vpdriver = {
	vpprobe, 0, 0, 0, 0, vpintr, 
	sizeof (struct vpdevice), "vp", vpdinfo, 0, 0, MDR_DMA,
};

vpprobe(reg)
	caddr_t reg;
{
	register struct vpdevice *vpaddr = (struct vpdevice *)reg;
	register int x;

	x = peek((short *)&vpaddr->vp_status);
	if (x == -1 || (x & VP_IS8237) == 0)
		return (0);
	if (poke((short *)&vpaddr->vp_cmd, VP_RESET))
		return (0);
	/* initialize 8259 so we don't get constant interrupts */
	vpaddr->vp_icad0 = 0x12;	/* ICW1, edge-trigger */
	DELAY(1);
	vpaddr->vp_icad1 = 0xFF;	/* ICW2 - don't care (non-zero) */
	DELAY(1);
	vpaddr->vp_icad1 = 0xFE;	/* IR0 - interrupt on DRDY edge */
	/* reset 8237 */
	vpaddr->vp_clear = 1;

	return (sizeof (struct vpdevice));
}

vpopen(dev)
	dev_t dev;
{
	register struct vp_softc *sc;
	register struct mb_device *md;
	register int s;
	static int vpwatch = 0;

	if (VPUNIT(dev) >= NVP ||
	    ((sc = &vp_softc[minor(dev)])->sc_state&VPSC_OPEN) ||
	    (md = vpdinfo[VPUNIT(dev)]) == 0 || md->md_alive == 0)
		return (ENXIO);
	if (!vpwatch) {
		vpwatch = 1;
		vptimo();
	}
	sc->sc_state = VPSC_OPEN|VPSC_PRINT | VPC_CLRCOM|VPC_RESET;
	while (sc->sc_state & VPSC_CMNDS) {
		s = splx(pritospl(md->md_intpri));
		if (vpwait(dev)) {
			vpclose(dev);
			(void) splx(s);
			return (EIO);
		}
		vpcmd(dev);
		(void) splx(s);
	}
	return (0);
}

vpclose(dev)
	dev_t dev;
{
	register struct vp_softc *sc = &vp_softc[VPUNIT(dev)];

	sc->sc_state = 0;
}

vpstrategy(bp)
	register struct buf *bp;
{
	register struct vp_softc *sc = &vp_softc[VPUNIT(bp->b_dev)];
	register struct mb_device *md = vpdinfo[VPUNIT(bp->b_dev)];
	register struct vpdevice *vpaddr = (struct vpdevice *)md->md_addr;
	int s;
	int pa, wc;
	
	if (((int)bp->b_un.b_addr & 1) || bp->b_bcount < 2) {
		bp->b_flags |= B_ERROR;
		iodone(bp);
		return;
	}
	s = splx(pritospl(md->md_intpri));
	while (sc->sc_bp != NULL)		/* single thread */
		(void) sleep((caddr_t)sc, VPPRI);
	sc->sc_bp = bp;
	(void) vpwait(bp->b_dev);
	sc->sc_mbinfo = mbsetup(md->md_hd, bp, 0);
	vpaddr->vp_clear = 1;
	pa = MBI_ADDR(sc->sc_mbinfo);
	vpaddr->vp_hiaddr = (pa >> 16) & 0xF;
	pa = (pa >> 1) & 0x7FFF;
	wc = (bp->b_bcount >> 1) - 1;
	bp->b_resid = 0;
	vpaddr->vp_addr = pa & 0xFF;
	vpaddr->vp_addr = pa >> 8;
	vpaddr->vp_wc = wc & 0xFF;
	vpaddr->vp_wc = wc >> 8;
	vpaddr->vp_mode = VP_DMAMODE;
	vpaddr->vp_clrmask = 1;
	sc->sc_state |= VPSC_BUSY;
	(void) splx(s);
}

/*ARGSUSED*/
vpwrite(dev, uio)
	dev_t dev;
	struct uio *uio;
{

	if (VPUNIT(dev) >= NVP)
		return (ENXIO);
	return (physio(vpstrategy, &rvpbuf[VPUNIT(dev)], dev, B_WRITE,
		    minphys, uio));
}

vpwait(dev)
	dev_t dev;
{
	register struct vpdevice *vpaddr =
	    (struct vpdevice *)vpdinfo[VPUNIT(dev)]->md_addr;
	register struct vp_softc *sc = &vp_softc[VPUNIT(dev)];

	for (;;) {
		if ((sc->sc_state & VPSC_BUSY) == 0 && 
		    vpaddr->vp_status & VP_DRDY)
			break;
		(void) sleep((caddr_t)sc, VPPRI);
	}
	return (0);	/* NO ERRORS YET */
}

struct pair {
	char	soft;		/* software bit */
	char	hard;		/* hardware bit */
} vpbits[] = {
	VPC_RESET,	VP_RESET,
	VPC_CLRCOM,	VP_CLEAR,
	VPC_EOTCOM,	VP_EOT,
	VPC_FFCOM,	VP_FF,
	VPC_TERMCOM,	VP_TERM,
	0,		0,
};

vpcmd(dev)
	dev_t;
{
	register struct vp_softc *sc = &vp_softc[VPUNIT(dev)];
	register struct vpdevice *vpaddr =
	    (struct vpdevice *)vpdinfo[VPUNIT(dev)]->md_addr;
	register struct pair *bit;

	for (bit = vpbits; bit->soft != 0; bit++) {
		if (sc->sc_state & bit->soft) {
			vpaddr->vp_cmd = bit->hard;
			sc->sc_state &= ~bit->soft;
			DELAY(100);	/* time for DRDY to drop */
			return;
		}
	}
}

/*ARGSUSED*/
vpioctl(dev, cmd, data, flag)
	dev_t dev;
	int cmd;
	caddr_t data;
	int flag;
{
	register int m;
	register struct mb_device *md = vpdinfo[VPUNIT(dev)];
	register struct vp_softc *sc = &vp_softc[VPUNIT(dev)];
	register struct vpdevice *vpaddr = (struct vpdevice *)md->md_addr;
	int s;

	switch (cmd) {

	case VGETSTATE:
		*(int *)data = sc->sc_state;
		break;

	case VSETSTATE:
		m = *(int *)data;
		sc->sc_state =
		    (sc->sc_state & ~VPSC_MODE) | (m&(VPSC_MODE|VPSC_CMNDS));
		break;

	default:
		return (ENOTTY);
	}
	s = splx(pritospl(md->md_intpri));
	(void) vpwait(dev);
	if (sc->sc_state&VPSC_SPP)
		vpaddr->vp_status = VP_SPP|VP_PLOT;
	else if (sc->sc_state&VPSC_PLOT)
		vpaddr->vp_status = VP_PLOT;
	else
		vpaddr->vp_status = 0;
	while (sc->sc_state & VPSC_CMNDS) {
		(void) vpwait(dev);
		vpcmd(dev);
	}
	(void) splx(s);
	return (0);
}

vpintr()
{
	register int dev;
	register struct mb_device *md;
	register struct vpdevice *vpaddr;
	register struct vp_softc *sc;
	register int found = 0;

	for (dev = 0; dev < NVP; dev++) {
		if ((md = vpdinfo[dev]) == NULL)
			continue;
		vpaddr = (struct vpdevice *)md->md_addr;
		vpaddr->vp_icad0 = VP_ICPOLL;
		DELAY(1);
		if (vpaddr->vp_icad0 & 0x80) {
			found = 1;
			DELAY(1);
			vpaddr->vp_icad0 = VP_ICEOI;
		}
		sc = &vp_softc[dev];
		if ((sc->sc_state&VPSC_BUSY) && (vpaddr->vp_status & VP_DRDY)) {
			sc->sc_state &= ~VPSC_BUSY;
			if (sc->sc_state & VPSC_SPP) {
				sc->sc_state &= ~VPSC_SPP;
				sc->sc_state |=  VPSC_PLOT;
				vpaddr->vp_status = VP_PLOT;
			}
			iodone(sc->sc_bp);
			sc->sc_bp = NULL;
			mbrelse(md->md_hd, &sc->sc_mbinfo);
		}
		wakeup((caddr_t)sc);
	}
	return (found);
}

vptimo()
{
	int s;
	register struct mb_device *md = vpdinfo[0];

	s = splx(pritospl(md->md_intpri));
	(void) vpintr();
	(void) splx(s);
	timeout(vptimo, (caddr_t)0, hz);
}
