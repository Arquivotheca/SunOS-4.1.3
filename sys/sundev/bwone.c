#ifndef lint
static char sccsid[] = "@(#)bwone.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright 1984, 1986, 1987 by Sun Microsystems, Inc.
 */

/*
 * Sun-1 Black & White Frame Buffer Driver
 */

#include "bwone.h"
#include "win.h"

#include <sys/param.h>
#include <sys/buf.h>
#include <sys/errno.h>
#include <sys/ioctl.h>

#include <sun/fbio.h>
#include <sundev/mbvar.h>

#include <pixrect/pixrect.h>
#include <pixrect/bw1reg.h>
#include <pixrect/bw1var.h>

#define BW1SIZE		(sizeof (struct bw1fb))

/* Mainbus device data */
int bwoneprobe(), bwoneintr();
struct mb_device *bwoneinfo[NBWONE];
struct mb_driver bwonedriver = {
	bwoneprobe, 0, 0, 0, 0, bwoneintr,
	BW1SIZE, "bwone", bwoneinfo, 0, 0, 0, 0
};

/* driver per-unit data */
struct bw1_softc {
	struct bw1fb *fb;	/* virtual address */
	int video;		/* video flags */
#if NWIN > 0
	Pixrect pr;		/* kernel pixrect and private data */
	struct bw1pr prd;
#endif NWIN > 0
} bw1_softc[NBWONE];

/* return structure for FBIOGTYPE ioctl */
static struct fbtype bw1_fbtype = {
	/* type, height, width, depth, cmsize, size */
	FBTYPE_SUN1BW, 800, 1024, 1, 2, BW1SIZE
};

#if NWIN > 0
/* SunWindows specific data */

/* kernel pixrect ops vector */
struct pixrectops bw1_ops = {
	bw1_rop,
	bw1_putcolormap,
	bw1_putattributes,
#	ifdef _PR_IOCTL_KERNEL_DEFINED
	0
#	endif
};

/* default pixrect and private data */
static Pixrect pr_default = {
	/* ops, size, depth, data */
	&bw1_ops, { 1024, 800 }, 1, 0
};

static struct bw1pr prd_default = {
	/* va, fd, flags, mask, offset */
	0, 0, BW_REVERSEVIDEO, 0, { 0, 0 }
};

#endif NWIN > 0

/*ARGSUSED*/
bwoneprobe(reg, unit)
	caddr_t reg;
	int unit;
{
	register struct bw1_softc *softc = &bw1_softc[unit];

	/*
 	 * We just do a peek of the first byte and don't bother trying
	 * to figure out if we're really talking to a bwone.  
 	 */
	if (peekc((char *) reg) == -1)
		return 0;

	/*
	 * The bwone frame buffer doesn't indicate that an interrupt
	 * is pending, and the interrupt level is user program changable.
	 * Thus, the kernel never knows what level to expect an
	 * interrupt from and doesn't know if an interrupt is pending.
	 * We add the bwoneintr routine to a list of interrupt handlers 
	 * that are called if no one else handles an interrupt.
	 */
	add_default_intr(bwoneintr);

	softc->fb = (struct bw1fb *) reg;
	softc->video = FBVIDEO_OFF;

	return BW1SIZE;
}

bwoneopen(dev, flag)
	dev_t dev;
	int flag;
{
	return fbopen(dev, flag, NBWONE, bwoneinfo);
}

/*ARGSUSED*/
bwoneclose(dev, flag)
	dev_t dev;
	int flag;
{
	return 0;
}

bwonemmap(dev, off, prot)
	dev_t dev;
	off_t off;
	int prot;
{
	return fbmmap(dev, off, prot, NBWONE, bwoneinfo, BW1SIZE);
}

/*ARGSUSED*/
bwoneioctl(dev, cmd, data, flag)
	dev_t dev;
	int cmd;
	caddr_t data;
	int flag;
{
	register struct bw1_softc *softc = &bw1_softc[minor(dev)];

	switch (cmd) {

	case FBIOGTYPE:
		* (struct fbtype *) data = bw1_fbtype;
		break;

#if NWIN > 0

	case FBIOGPIXRECT:
		((struct fbpixrect *) data)->fbpr_pixrect = &softc->pr;

		/* initialize pixrect */
		softc->pr = pr_default;
		softc->pr.pr_data = (caddr_t) &softc->prd;

		/* initialize private data */
		softc->prd = prd_default;
		softc->prd.bwpr_va = softc->fb;

		/* enable video, disable and clear interrupt */
		softc->video = FBVIDEO_ON;
		bw1_setcontrol(softc->fb, BWCONTROL_VIDEOENABLE);
		bw1_intclear(softc->fb);

		break;

#endif NWIN > 0

	case FBIOSVIDEO: 
		bw1_setcontrol(softc->fb, 
			(softc->video = * (int *) data) & FBVIDEO_ON ? 
			BWCONTROL_VIDEOENABLE : 0);
		break;

	case FBIOGVIDEO: 
		* (int *) data = softc->video;
		break;

	default:
		return ENOTTY;
	}

	return 0;
}

static
bwoneintclear(addr)
	caddr_t addr;
{
	/*
	 * Turn off interrupts, then return 0 since we have no way of
	 * knowing whether we caused the interrupt.
	 */
	bw1_intclear((struct bw1fb *) addr);

	return 0;
}

bwoneintr()
{
	return fbintr(NBWONE, bwoneinfo, bwoneintclear);
}
