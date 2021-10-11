#ifndef lint
static char sccsid[] = "@(#)cgtwo.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright 1986, 1987 by Sun Microsystems, Inc.
 */

/*
 * Sun-2/Sun-3 color board driver
 */

#include "cgtwo.h"
#include "win.h"

#include <sys/param.h>
#include <sys/buf.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/map.h>
#include <sys/vmmac.h>
#include <sys/time.h>
#include <sys/kernel.h>

#include <sun/fbio.h>

#include <machine/pte.h>
#include <machine/mmu.h>
#include <machine/psl.h>
#ifdef sun3x
#include <machine/cpu.h>
#endif sun3x

#include <sundev/mbvar.h>

#include <pixrect/pixrect.h>
#include <pixrect/pr_impl_util.h>
#include <pixrect/memreg.h>
#include <pixrect/cg2reg.h>
#include <pixrect/cg2var.h>

/* mapped (probed) and total size of board */
#define	CG2_PROBESIZE	CG2_MAPPED_SIZE
#define	CG2_TOTAL_SIZE	(CG2_MAPPED_OFFSET + CG2_MAPPED_SIZE)

/* Mainbus device data */
int cgtwoprobe(), cgtwoattach();

struct mb_device *cgtwoinfo[NCGTWO];
struct mb_driver cgtwodriver = {
	cgtwoprobe, 0, cgtwoattach, 0, 0, 0,
	CG2_PROBESIZE, "cgtwo", cgtwoinfo, 0, 0, 0, 0
};

/* driver per-unit data */
struct cg2_softc {
	int flags;		/* misc. flags; bits defined in cg2var.h */
				/* (struct cg2pr, flags member) */
	struct cg2fb *fb;	/* virtual address */
	int w, h;		/* resolution */
#if NWIN > 0
	Pixrect pr;		/* kernel pixrect and private data */
	struct cg2pr prd;
#endif NWIN > 0
} cg2_softc[NCGTWO];

/* default structure for FBIOGATTR/FBIOGTYPE ioctls */
static struct fbgattr fbgattr_default =  {
/*	real_type         owner */
	FBTYPE_SUN2COLOR, 0,
/* fbtype: type             h  w  depth cms  size */
	{ FBTYPE_SUN2COLOR, 0, 0, 8,    256, CG2_TOTAL_SIZE },
/* fbsattr: flags         emu_type */
	{ FB_ATTR_DEVSPECIFIC, -1, 
/* dev_specific:   FLAGS,      BUFFERS, PRFLAGS */
	{ FB_ATTR_CG2_FLAGS_PRFLAGS, 1, 0 } },
/* emu_types */
	{ -1, -1, -1, -1}
};

/* double buffering enable flag */
int cg2_dblbuf_enable = 1;

#if NWIN > 0

/* SunWindows specific stuff */

/* kernel pixrect ops vector */
struct pixrectops cg2_ops = {
	cg2_rop,
	cg2_putcolormap,
	cg2_putattributes,
#	ifdef _PR_IOCTL_KERNEL_DEFINED
	0
#	endif
};

/* need gp1_sync stub if no GPs configured */
#include "gpone.h"
#if NGPONE == 0
gp1_sync() { return -1; }
#endif NGPONE == 0

#endif NWIN > 0


cgtwoprobe(reg, unit)
	caddr_t reg;
	int unit;
{
	register struct cg2fb *fb = (struct cg2fb *) reg;
	register struct cg2_softc *softc;

	/* 
	 * Config address is base address of board, so we actually
	 * have the plane mode memory (and a little bit of pixel mode)
	 * mapped at this point.  Re-map 2M higher to get the rasterop
	 * mode memory and control registers mapped.
	 */
	fbmapin((caddr_t) fb, fbgetpage((caddr_t) fb) +
		(int) btop(CG2_MAPPED_OFFSET), CG2_MAPPED_SIZE);

	if (probeit(fb))
		return 0;

	softc = &cg2_softc[unit];
	softc->fb = fb;
	softc->flags = 0;

	/* check for supported resolution */
	switch (fb->status.reg.resolution) {
	case CG2_SCR_1152X900:
		softc->w = 1152; 
		softc->h =  900;
		softc->flags = CG2D_STDRES;
		break;
	case CG2_SCR_1024X1024:
		softc->w = 1024;
		softc->h = 1024;
		break;
	default:
		printf("%s%d: unsupported resolution (%d)\n",
			cgtwodriver.mdr_cname, unit,
			fb->status.reg.resolution);
		return 0;
	}

	return CG2_PROBESIZE;
}

static 
probeit(fb)
	register struct cg2fb *fb;
{
	union {
		struct cg2statusreg reg;
		short word;
	} status;

#define	allrop(fb, reg)	((short *) &(fb)->ropcontrol[CG2_ALLROP].ropregs.reg)
#define	pixel0(fb)	((char *) &fb->ropio.roppixel.pixel[0][0])

	/*
	 * Probe sequence:
	 *
	 * set board for pixel mode access
	 * enable all planes
	 * set rasterop function to CG_SRC
	 * disable end masks
	 * set fifo shift/direction to zero/left-to-right
	 * write 0xa5 to pixel at (0,0)
	 * check pixel value
	 * enable subset of planes (0xcc)
	 * set rasterop function to ~CG_DEST
	 * write to pixel at (0,0) again
	 * enable all planes again
	 * read pixel value; should be 0xa5 ^ 0xcc = 0x69
	 */
	status.word = peek(&fb->status.word);
	status.reg.ropmode = SWWPIX;
	if (poke(&fb->status.word, status.word) ||
		poke((short *) &fb->ppmask.reg, 255) ||
		poke(allrop(fb, mrc_op), CG_SRC) ||
		poke(allrop(fb, mrc_mask1), 0) ||
		poke(allrop(fb, mrc_mask2), 0) ||
		poke(allrop(fb, mrc_shift), 1 << 8) ||
		pokec(pixel0(fb), 0xa5) ||
		pokec(pixel0(fb), 0) ||
		peekc(pixel0(fb)) != 0xa5 ||
		poke((short *) &fb->ppmask.reg, 0xcc) ||
		poke(allrop(fb, mrc_op), ~CG_DEST) ||
		pokec(pixel0(fb), 0) ||
		poke((short *) &fb->ppmask.reg, 255) ||
		peekc(pixel0(fb)) != (0xa5 ^ 0xcc))
		return 1;

	return 0;

#undef	allrop
#undef	pixel0
}


cgtwoattach(md)
	struct mb_device *md;
{
	register struct cg2_softc *softc = &cg2_softc[md->md_unit];
	register struct cg2fb *fb = softc->fb;
	register int flags = softc->flags;

#define	dummy	flags

	/* set interrupt vector */
	if (md->md_intr)
		fb->intrptvec.reg = md->md_intr->v_vec;
	else
		printf(
		"WARNING: no interrupt vector specified in config file\n");

	/* 
	 * Determine whether this is a Sun-2 or Sun-3 color board
	 * by setting the wait bit in the double buffering register
	 * and seeing if it clears itself during retrace.
	 *
	 * On the Sun-2 color board this just writes a bit in the
	 * "wordpan" register.
	 */
	fb->misc.nozoom.dblbuf.word = 0;
	fb->misc.nozoom.dblbuf.reg.wait = 1;

	/* wait for leading edge, then trailing edge of retrace */
	while (fb->status.reg.retrace)
		/* nothing */ ;
	while (!fb->status.reg.retrace)
		/* nothing */ ;
	while (fb->status.reg.retrace)
		/* nothing */ ;

	if (fb->misc.nozoom.dblbuf.reg.wait) {
		/* Sun-2 color board */
		fb->misc.nozoom.dblbuf.reg.wait = 0;
		flags |= CG2D_ZOOM;
	}
	else {
		/* Sun-3 color board (or better) */
		flags |= CG2D_32BIT | CG2D_NOZOOM;

		if (fb->status.reg.fastread)
			flags |= CG2D_FASTREAD;

		if (fb->status.reg.id) 
			flags |= CG2D_ID | CG2D_ROPMODE;

		/*
		 * Probe for double buffering feature.
		 * Write distinctive values to one pixel in both buffers,
		 * then two pixels in buffer B only.
		 * Read from buffer B and see what we get.
		 *
		 * Warning: assumes we were called right after cgtwoprobe.
		 */
		cg2_setfunction(fb, CG2_ALLROP, CG_SRC);
		fb->ropio.roppixel.pixel[0][0] = 0x5a;
		fb->ropio.roppixel.pixel[0][0] = 0xa5;
		fb->misc.nozoom.dblbuf.reg.nowrite_a = 1;
		fb->ropio.roppixel.pixel[0][0] = 0xc3;
		fb->ropio.roppixel.pixel[0][4] = dummy;
		if (fb->ropio.roppixel.pixel[0][0] == 0x5a) {
			fb->misc.nozoom.dblbuf.reg.read_b = 1;

			if (fb->ropio.roppixel.pixel[0][0] == 0xa5 &&
				fb->ropio.roppixel.pixel[0][4] == 0xc3 &&
				cg2_dblbuf_enable)
				flags |= CG2D_DBLBUF;
		}
		fb->misc.nozoom.dblbuf.word = 0;
	}

	softc->flags = flags;

#ifndef sun2
	/* re-map into correct VME space if necessary */
	{
		int page = fbgetpage((caddr_t) fb);
#ifndef sun3x

		if (((flags & CG2D_32BIT) != 0) !=
			((page & PGT_MASK) == PGT_VME_D32))
			fbmapin((caddr_t) fb, 
				page ^ (PGT_VME_D16 ^ PGT_VME_D32),
				CG2_MAPPED_SIZE);
#endif !sun3x
#ifdef sun3x

		if (page < mmu_btop(VME24D32_BASE) &&
		    ((flags & CG2D_32BIT) != 0))
			fbmapin((caddr_t)fb,
			    (int) (page - mmu_btop(VME24D16_BASE) +
			    mmu_btop(VME24D32_BASE)), CG2_MAPPED_SIZE);
		else if (page >= mmu_btop(VME24D32_BASE) &&
		    ((flags & CG2D_32BIT) == 0))
			fbmapin((caddr_t)fb,
			    (int) (page - mmu_btop(VME24D32_BASE) +
			    mmu_btop(VME24D16_BASE)), CG2_MAPPED_SIZE);
#endif sun3x
	}
#endif !sun2

	/* print informative message */
	printf("%s%d: Sun-%c color board%s%s\n", 
		md->md_driver->mdr_dname, md->md_unit,
		flags & CG2D_ZOOM ? '2' : '3',
		flags & CG2D_DBLBUF ? ", double buffered" : "",
		flags & CG2D_FASTREAD ? ", fast read" : "");
}

cgtwoopen(dev, flag)
	dev_t dev;
	int flag;
{
	return fbopen(dev, flag, NCGTWO, cgtwoinfo);
}

/*ARGSUSED*/
cgtwoclose(dev, flag)
	dev_t dev;
{
	register struct cg2_softc *softc = &cg2_softc[minor(dev)];
	register struct cg2fb *fb = softc->fb;

	/* fix up zoom and/or double buffering on close */

	if (softc->flags & CG2D_ZOOM) {
		fb->misc.zoom.wordpan.reg = 0;	/* hi pixel adr = 0 */
		fb->misc.zoom.zoom.word = 0;	/* zoom=0, yoff=0 */
		fb->misc.zoom.pixpan.word = 0;	/* pix adr=0, xoff=0 */
		fb->misc.zoom.varzoom.reg = 255; /* unzoom at line 4*255 */
	}

	if (softc->flags & CG2D_NOZOOM) 
		fb->misc.nozoom.dblbuf.word = 0;

	return 0;
}

/*ARGSUSED*/
cgtwommap(dev, off, prot)
	dev_t dev;
	off_t off;
	int prot;
{
	if ((u_int) off >= CG2_TOTAL_SIZE)
		return -1;

	return fbgetpage((caddr_t) cg2_softc[minor(dev)].fb) + 
		btop(off) - btop(CG2_MAPPED_OFFSET);
}

/*ARGSUSED*/
cgtwoioctl(dev, cmd, data, flag)
	dev_t dev;
	int cmd;
	caddr_t data;
	int flag;
{
	register struct cg2_softc *softc = &cg2_softc[minor(dev)];

	switch (cmd) {

	case FBIOGTYPE: {
		register struct fbtype *fbtype = (struct fbtype *) data;

		*fbtype = fbgattr_default.fbtype;
		fbtype->fb_height = softc->h;
		fbtype->fb_width  = softc->w;
	}
	break;

	case FBIOGATTR: {
		register struct fbgattr *gattr = (struct fbgattr *) data;

		*gattr = fbgattr_default;
		gattr->fbtype.fb_height = softc->h;
		gattr->fbtype.fb_width = softc->w;

		if (softc->flags & CG2D_NOZOOM)
			gattr->sattr.dev_specific[FB_ATTR_CG2_FLAGS] |=
				FB_ATTR_CG2_FLAGS_SUN3;

		if (softc->flags & CG2D_DBLBUF)
			gattr->sattr.dev_specific[FB_ATTR_CG2_BUFFERS] = 2;

		gattr->sattr.dev_specific[FB_ATTR_CG2_PRFLAGS] = softc->flags;
	}
	break;

	case FBIOSATTR: 
		break;

#if NWIN > 0

	case FBIOGPIXRECT: 
		((struct fbpixrect *) data)->fbpr_pixrect = &softc->pr;

		/* initialize pixrect */
		softc->pr.pr_ops = &cg2_ops;
		softc->pr.pr_size.x = softc->w;
		softc->pr.pr_size.y = softc->h;
		softc->pr.pr_depth = CG2_DEPTH;
		softc->pr.pr_data = (caddr_t) &softc->prd;

		/* initialize private data */
		bzero((char *) &softc->prd, sizeof softc->prd);
		softc->prd.cgpr_va = softc->fb;
		softc->prd.cgpr_fd = 0;
		softc->prd.cgpr_planes = 255;
		softc->prd.ioctl_fd = minor(dev);
		softc->prd.flags = softc->flags;
		softc->prd.linebytes = softc->w;

		/* enable video */
		softc->fb->status.reg.video_enab = 1;

		break;

#endif NWIN > 0

	/* get info for GP */
	case FBIOGINFO: {
		register struct fbinfo *fbinfo = (struct fbinfo *) data;

		fbinfo->fb_physaddr = 
			(fbgetpage((caddr_t) softc->fb) << PGSHIFT) -
			CG2_MAPPED_OFFSET & 0xffffff;
		fbinfo->fb_hwwidth = softc->w;
		fbinfo->fb_hwheight = softc->h;
		fbinfo->fb_ropaddr = (u_char *) softc->fb;
	}
	break;

	/* set video flags */
	case FBIOSVIDEO: 
		softc->fb->status.reg.video_enab = 
			(* (int *) data) & FBVIDEO_ON ? 1 : 0;
		break;

	/* get video flags */
	case FBIOGVIDEO: 
		* (int *) data = softc->fb->status.reg.video_enab
			? FBVIDEO_ON : FBVIDEO_OFF;
		break;

	case FBIOVERTICAL: 
		cgtwo_wait(minor(dev));
		break;

	default:
		return ENOTTY;
	}

	return 0;
	}

/*
 * This oneshot timeout is needed to ensure that requested vertical
 * retrace interrupts are serviced.  It works around a problem exhibited
 * by pixrects on sparc platforms:  pixrects code performs exclusive-or
 * operations on the ropmode bits in the status register.  On the sparc,
 * the exclusive-or requires three instructions, ie it is nonatomic, and
 * can/does get interrupted in the middle of the 3-instruction sequence.
 */
static
cgtwotimeout(unit)
int unit;
{
        register struct cg2_softc *softc = &cg2_softc[unit & 255];
        register struct mb_device *md    = cgtwoinfo[unit & 255];
        int s;

        s = splx(pritospl(md->md_intpri));

        softc->fb->status.reg.inten = 1;
        timeout(cgtwotimeout, (caddr_t) (unit & 255), hz);

        (void) splx(s);
}

/* wait for vertical retrace interrupt */
cgtwo_wait(unit)
	int unit;
{
	register struct mb_device *md = cgtwoinfo[unit & 255];
	register struct cg2_softc *softc = &cg2_softc[unit & 255];
	int s;

	if (md->md_intr == 0)
		return;

	s = splx(pritospl(md->md_intpri));
	softc->fb->status.reg.inten = 1;

        /* see comments on cgtwotimeout() */
        timeout(cgtwotimeout, (caddr_t) (unit & 255), hz);

	(void) sleep((caddr_t) softc, PZERO - 1);
	(void) splx(s);
}

/* vertical retrace interrupt service routine */
cgtwointr(unit)
	int unit;
{
	register struct cg2_softc *softc = &cg2_softc[unit];

	untimeout(cgtwotimeout, (caddr_t) unit);

	softc->fb->status.reg.inten = 0;
	wakeup((caddr_t) softc);

#ifdef lint
	cgtwointr(unit);
#endif
}
