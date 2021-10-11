#ifndef lint
static char sccsid[] = "@(#)cgone.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright 1983, 1986, 1987 by Sun Microsystems, Inc.
 */

/*
 * Sun One Color Graphics Board Driver
 */

#include "cgone.h"
#include "win.h"

#include <sys/param.h>
#include <sys/buf.h>
#include <sys/errno.h>
#include <sys/ioctl.h>

#include <sun/fbio.h>
#include <sundev/mbvar.h>

#include <pixrect/pixrect.h>
#include <pixrect/cg1reg.h>
#include <pixrect/cg1var.h>

/* handy register access macro */
#define	cg1_reg(fb, r)	((fb)->update[0].regsel.reg[(r)].dat[0])

#define	CG1SIZE		(sizeof (struct cg1fb))

/* Mainbus device data */
int cgoneprobe(), cgoneintr();
struct mb_device *cgoneinfo[NCGONE];
struct mb_driver cgonedriver = {
	cgoneprobe, 0, 0, 0, 0, cgoneintr,
	CG1SIZE, "cgone", cgoneinfo, 0, 0, 0, 0
};

/* driver per-unit data */
struct cg1_softc {
	struct cg1fb *fb;	/* virtual address */
#if NWIN > 0
	Pixrect pr;		/* kernel pixrect and private data */
	struct cg1pr prd;
#endif NWIN > 0
} cg1_softc[NCGONE];

/* return structure for FBIOGTYPE ioctl */
static struct fbtype cg1_fbtype = {
	/* type, height, width, depth, cmsize, size */
	FBTYPE_SUN1COLOR, CG1_HEIGHT, CG1_WIDTH, CG1_DEPTH, 
	1 << CG1_DEPTH, CG1SIZE
};

#if NWIN > 0
/* SunWindows specific data */

/* kernel pixrect ops vector */
struct pixrectops cg1_ops = {
	cg1_rop,
	cg1_putcolormap,
	cg1_putattributes,
#	ifdef _PR_IOCTL_KERNEL_DEFINED
	0
#	endif
};

/* default pixrect and private data */
static Pixrect pr_default = {
	/* ops, size, depth, data */
	&cg1_ops, { CG1_WIDTH, CG1_HEIGHT }, CG1_DEPTH, 0
};

static struct cg1pr prd_default = {
	/* va, fd, planes, offset */
	0, 0, 255, { 0, 0 }
};

#endif NWIN > 0

/*ARGSUSED*/
cgoneprobe(reg, unit)
	caddr_t reg;
	int unit;
{
	register struct cg1fb *fb = (struct cg1fb *) reg;
	register char *xaddr;

	/*
	 * Probe sequence:
	 * set function to invert
	 * write a pixel
	 * read it back and see if it was inverted
	 */
#define DATA1 0x5C
#define DATA2 0x33

	xaddr = (char *) cg1_xpipe(fb, 0, 1, 0);

	if (pokec((char *) &cg1_reg(fb, CG_FUNCREG), ~CG_SRC) ||
		pokec((char *) &cg1_reg(fb, CG_MASKREG), 0) ||
		pokec((char *) cg1_ysrc(fb, 0), 0) ||
		pokec(xaddr, DATA1) ||
		peekc(xaddr) == -1 ||
		pokec(xaddr, DATA2) ||
		peekc(xaddr) != (DATA1 ^ 0xff))
		return 0;

	/*
	 * The bwone frame buffer doesn't indicate that an interrupt
	 * is pending, and the interrupt level is user program changable.
	 * Thus, the kernel never knows what level to expect an
	 * interrupt from and doesn't know if an interrupt is pending.
	 * We add the bwoneintr routine to a list of interrupt handlers 
	 * that are called if no one else handles an interrupt.
	 */
	add_default_intr(cgoneintr);

	cg1_softc[unit].fb = fb;

	return CG1SIZE;

#undef	DATA1
#undef	DATA2
}

cgoneopen(dev, flag)
	dev_t dev;
	int flag;
{
	return fbopen(dev, flag, NCGONE, cgoneinfo);
}

/*ARGSUSED*/
cgoneclose(dev, flag)
	dev_t dev;
	int flag;
{
	return 0;
}

cgonemmap(dev, off, prot)
	dev_t dev;
	off_t off;
	int prot;
{
	return fbmmap(dev, off, prot, NCGONE, cgoneinfo, CG1SIZE);
}

/*ARGSUSED*/
cgoneioctl(dev, cmd, data, flag)
	dev_t dev;
	int cmd;
	caddr_t data;
	int flag;
{
	register struct cg1_softc *softc = &cg1_softc[minor(dev)];

	switch (cmd) {

	case FBIOGTYPE:
		* (struct fbtype *) data = cg1_fbtype;
		break;

#if NWIN > 0

	case FBIOGPIXRECT:
		((struct fbpixrect *) data)->fbpr_pixrect = &softc->pr;

		/* initialize pixrect */
		softc->pr = pr_default;
		softc->pr.pr_data = (caddr_t) &softc->prd;

		/* initialize private data */
		softc->prd = prd_default;
		softc->prd.cgpr_va = softc->fb;

		/* enable video, disable interrupt */
		cg1_setcontrol(softc->fb, CG_VIDEOENABLE);

		break;

#endif NWIN > 0

	case FBIOSVIDEO: 
		if (* (int *) data & FBVIDEO_ON)
			cg1_reg(softc->fb, CG_STATUS) |= CG_VIDEOENABLE;
		else
			cg1_reg(softc->fb, CG_STATUS) &= ~CG_VIDEOENABLE;
		break;

	case FBIOGVIDEO:
		* (int *) data = 
			cg1_reg(softc->fb, CG_STATUS) & CG_VIDEOENABLE ?
			FBVIDEO_ON : FBVIDEO_OFF;
		break;

	default:
		return ENOTTY;
	}

	return 0;
}

static
cgoneintclear(addr)
	caddr_t addr;
{
	/*
	 * Turn off interrupts, then return 0 since we have no way of
	 * knowing whether we caused the interrupt.
	 */
	cg1_intclear((struct cg1fb *) addr);

	return 0;
}

cgoneintr()
{
	return fbintr(NCGONE, cgoneinfo, cgoneintclear);
}
