#ifndef lint
static char sccsid[] = "@(#)bwtwo.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright 1986, 1987 by Sun Microsystems, Inc.
 */

/*
 * Sun Two Black & White Board(s) Driver
 */

#include "bwtwo.h"
#if NBWTWO > 0
#include "win.h"

#include <sys/param.h>
#include <sys/buf.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/map.h>
#include <sys/vmmac.h>

#include <sun/fbio.h>

#include <machine/pte.h>
#include <machine/mmu.h>
#include <machine/cpu.h>

#include <sundev/mbvar.h>

#include <pixrect/pixrect.h>
#include <pixrect/memvar.h>
#include <sundev/bw2reg.h>
#include <pixrect/bw2var.h>

#ifndef sun2
#include <machine/eeprom.h>
#include <machine/enable.h>

#include <sundev/p4reg.h>

#ifdef sun3x
extern u_short enablereg;
#else
extern u_char enablereg;
#endif sun3x
#endif !sun2

/* macros */

#ifdef sun2
#define BW2_PROBESIZE	(BW2_FBSIZE + sizeof (struct bw2cr))
#else
#define	BW2_PROBESIZE	(BW2_FBSIZE_HIRES + NBPG)
#endif

/*
 * Driver information for auto-configuration stuff.
 */
int	bwtwoprobe(), bwtwoattach(), bwtwointr();
extern struct	mb_device *bwtwoinfo[];
struct	mb_driver bwtwodriver = {
	bwtwoprobe, 0, bwtwoattach, 0, 0, bwtwointr,
	BW2_PROBESIZE, "bwtwo", bwtwoinfo, 0, 0, 0,
};

#if NWIN > 0

/* SunWindows specific stuff */

struct pixrectops bw2_ops = {
	mem_rop,
	mem_putcolormap,
	mem_putattributes,
#	ifdef _PR_IOCTL_KERNEL_DEFINED
	0
#	endif
};

#endif NWIN > 0

extern struct bw2_softc bw2_softc[];

bwtwoopen(dev, flag)
	dev_t dev;
	int flag;
{
	extern int nbwtwo;
	return fbopen(dev, flag, nbwtwo, bwtwoinfo);
}

/*ARGSUSED*/
bwtwoclose(dev, flag)
	dev_t dev;
	int flag;
{
	return 0;
}

/*ARGSUSED*/
bwtwommap(dev, off, prot)
	dev_t dev;
	off_t off;
	int prot;
{
	register struct bw2_softc *softc = &bw2_softc[minor(dev)];

	if (off >= (off_t) softc->size)
		return -1;

	return fbgetpage(softc->image + off);
}

/*
 * Determine if a bwtwo exists at the given address.
 * If it exists, determine its size.
 */
/*ARGSUSED*/
bwtwoprobe(reg, unit)
	caddr_t reg;
	int	unit;
{
	register struct bw2_softc *softc = &bw2_softc[unit];

	struct size_tab {
#ifndef sun2
		char sizecode;
#endif !sun2
		int w, h;
		int size;
	};

#ifdef lint
#define	SIZE_TAB_ENT(w,h)	{ 0 }
#else lint
#ifdef sun2 
#define	SIZE_TAB_ENT(w,h) \
	{ w, h, mpr_linebytes(w, 1) * h }
#else sun2
#define	SIZE_TAB_ENT(w,h) \
	{ EED_SCR_/**/w/**/X/**/h, w, h, mpr_linebytes(w, 1) * h }
#endif sun2
#endif lint

	/* first table entry is default size */
	static struct size_tab size_tab[] = {
		SIZE_TAB_ENT(1152,900),
		SIZE_TAB_ENT(1024,1024),
#ifndef sun2
		SIZE_TAB_ENT(1600,1280),
		SIZE_TAB_ENT(1440,1440)
#endif !sun2
	};
#undef	SIZE_TAB_ENT

	register struct size_tab *sizep = size_tab;

	if (softc->image || peek((short *) reg) == -1)
		return 0;

#ifndef sun2
	{
		/*
		 * Determine resolution:
		 * 
		 * 1. If this is a P4 frame buffer, use the P4 type code.
		 * 2. If it's unit 0:
		 *	a. look at the code in the EEPROM.
		 *	b. on 3/60s only, look at the resolution jumper
		 * 3. Otherwise, punt and assume 1152 x 900.
		 */

		int id, w, h;

		if ((id = p4probe(reg, &w, &h)) >= 0) {
			if (id != P4_ID_BW && 
				id != P4_ID_COLOR8P1)
				return 0;

			softc->p4reg = (u_long *) reg;
			softc->w = w;
			softc->h = h;

			fbmapin(softc->image = reg + NBPG,
				fbgetpage(reg) + (int) (id == P4_ID_BW ? 
					btop(P4_BW_OFF) : 
					btop(P4_COLOR_OFF_OVERLAY)),
				softc->size = 
					pr_product(mpr_linebytes(w, 1), h));

			return softc->size;
		}

		softc->p4reg = 0;
		sizep = size_tab;

		if (unit == 0) {
			for (; sizep->sizecode != EEPROM->ee_diag.eed_scrsize;
				sizep++)
				if (sizep >= &size_tab[sizeof(size_tab) /
					sizeof(size_tab[0])]) {
					sizep = size_tab;
					break;
				}
#ifdef SUN3_60
			/*
			 * The Sun-3/60 on-board frame buffer has a 
			 * resolution jumper.  We map it to the spare page
			 * after the frame buffer image and probe it.
			 *
			 * Even if the EEPROM is wrong about the resolution,
			 * we still go along with its idea of whether the
			 * monitor is normal or square.
			 */
			if (cpu == CPU_SUN3_60) {
				fbmapin(reg + BW2_FBSIZE_HIRES,
					(int) (BW2_RES_SENSE_PGT | 
					btop(BW2_RES_SENSE_ADDR)), 1);
				if ((id = peekc(reg + BW2_FBSIZE_HIRES)) >= 0)
					if (id & BW2_RES_SENSE_LOWRES) {
						if (sizep->h > 1152)
							sizep -= 2;
					}
					else
						if (sizep->w <= 1152)
							sizep += 2;
			}
#endif SUN3_60
		}

#ifdef SUN3_50
	/*
 	 * If we are on a SUN3_50 then we must reserve
	 * the on board memory for the frame buffer.
 	 */
	if (cpu == CPU_SUN3_50)
		if (fbobmemavail)
			fbobmemavail = 0;
		else
			return 0;
#endif SUN3_50

	}
#else !sun2
	{
		struct	bw2dev *bw2dev = (struct bw2dev *) reg;
		register struct	bw2cr *alias1, *alias2;
		short w1, w2, wrestore;

		bw2crmapin(bw2dev);
		alias1 = &bw2dev->bw2cr;
		alias2 = alias1 + 1;

		/*
	 	 * Two adjacent shorts should be the same because
	 	 * the control register is replicated every 2 bytes
	 	 * throughout the control page.
	 	 */
		if ((w1 = peek((short *) alias1)) == -1)
			return 0;
		wrestore = w1;

		((struct bw2cr *) &w1)->vc_copybase = 0xAA & BW2_COPYBASEMASK;

		if (poke((short *) alias1, w1) ||
			(w2 = peek((short *) alias2)) == -1 ||
			w1 != w2) {
				(void) poke((short *) alias1, wrestore);
				return 0;
		}

		((struct bw2cr *) &w1)->vc_copybase = ~0xAA & BW2_COPYBASEMASK;

		if (poke((short *) alias1, w1) ||
			(w2 = peek((short *) alias2)) == -1 ||
			w1 != w2) {
				(void) poke((short *) alias1, wrestore);
				return 0;
		}

		if (poke((short *) alias1, wrestore))
			panic("bwtwoprobe");

		if (cpu != CPU_SUN2_120 &&
			((struct bw2cr *) &wrestore)->vc_1024_jumper)
			sizep++;

		softc->cr = alias1;
	}
#endif sun2

	softc->image = reg;
	softc->w = sizep->w;
	softc->h = sizep->h;

	return softc->size = sizep->size;
}

bwtwoattach(md)
	register struct mb_device *md;
{
	register struct bw2_softc *softc = &bw2_softc[md->md_unit];

	printf("%s%d: resolution %d x %d\n",
		md->md_driver->mdr_dname, md->md_unit,
		softc->w, softc->h);

#if BW2_COPY_MEM_AVAIL
	{
		/* pfnum before shadow memory mapped in */
		static int copyenpfnum;	

		/* virtual address mapped to shadow memory */
		static caddr_t copyenvirt = 0;

		int	pfnum;
		caddr_t	fbvirtaddr;
		caddr_t	v;
		int	i;
		int	s;
		extern char CADDR1[];

		pfnum = fbgetpage(md->md_addr);

		/*
	 	 * Have we passed this way before? 
	 	 */
		if (fbobmemavail == 0) {
			if (copyenvirt == 0) {
				copyenvirt = (caddr_t)(*romp->v_fbaddr);
				if (pfnum == copyenpfnum)
					softc->image = copyenvirt;
			}
			return;
		}

		/* 
	 	 * We know that the copy memory can be used.  Use the
	 	 * shadow memory if the config flags say to use it.
	 	 */
#ifdef sun3
		if (md->md_flags & BW2_USECOPYMEM && cpu != CPU_SUN3_160) {
				printf(
			"WARNING: copy memory not available on this CPU\n"
					);
			md->md_flags &= ~BW2_USECOPYMEM;
		}
#endif sun3

		if (!(md->md_flags & BW2_USECOPYMEM)) {
			/* don't bother using reserved shadow memory */
			copyenvirt = md->md_addr;
			return;
		}

		/*
	 	 * Mark the onboard frame buffer memory as not available
	 	 * anymore as we are going to use it for copy memory.
	 	 */
		fbobmemavail = 0;

		/*
		 * If this frame buffer is the console, throw away
		 * config's mapping and use the boot PROM's.
		 */
		if (md->md_unit == 0 &&
			*romp->v_outsink == OUTSCREEN &&
			*romp->v_fbtype == FBTYPE_SUN2BW)
			fbvirtaddr = (caddr_t) md->md_addr;
		else {
			s = splhigh();
			rmfree(kernelmap, (long) btoc(softc->size), 
				(u_long) btokmx((struct pte *) md->md_addr));
			(void)splx(s);
			fbmapout(md->md_addr, softc->size);
			fbvirtaddr = (caddr_t) (*romp->v_fbaddr);
		} 
		copyenvirt = fbvirtaddr;
		copyenpfnum = fbgetpage(fbvirtaddr);

		/*
	 	 * Copy the current frame buffer memory
	 	 * to the copy memory as we map it in.
	 	 */
		for (v = (caddr_t) fbvirtaddr, i = btop(OBFBADDR);
			i < btop(OBFBADDR + softc->size); v += NBPG, i++) {
			mapin(CMAP1, btop(CADDR1),
				(u_int) (PGT_OBMEM | i), 1, PG_V | PG_KW);
			bcopy(v, CADDR1, NBPG);
			fbmapin(v, PGT_OBMEM | i, NBPG);
		}

#ifdef sun2
		(void) bwtwosetcr(softc->cr, 
			(OBFBADDR>>BW2_COPYSHIFT) | BW2_COPYENABLEMASK, 1);
#else sun2
		(void) setcopyenable(1);
#endif sun2

		if (pfnum == copyenpfnum)
			softc->image = copyenvirt;
	}
#endif BW2_COPY_MEM_AVAIL
}

/*ARGSUSED*/
bwtwoioctl(dev, cmd, data, flag)
	dev_t dev;
	int cmd;
	caddr_t data;
	int flag;
{
	register struct bw2_softc *softc = &bw2_softc[minor(dev)];

	switch (cmd) {

	case FBIOGTYPE: {
		register struct fbtype *fb = (struct fbtype *) data;

		fb->fb_type = FBTYPE_SUN2BW;
		fb->fb_height = softc->h;
		fb->fb_width = softc->w;
		fb->fb_depth = 1;
		fb->fb_cmsize = 2;
		fb->fb_size = softc->size;
	}
	break;

#if NWIN > 0

	case FBIOGPIXRECT: {
		((struct fbpixrect *) data)->fbpr_pixrect = &softc->pr;

		softc->pr.pr_ops = &bw2_ops;
		softc->pr.pr_size.x = softc->w;
		softc->pr.pr_size.y = softc->h;
		softc->pr.pr_depth = 1;
		softc->pr.pr_data = (caddr_t) &softc->prd;

		softc->prd.md_linebytes = mpr_linebytes(softc->w, 1);
		softc->prd.md_image = (short *) softc->image;
		softc->prd.md_offset.x = 0;
		softc->prd.md_offset.y = 0;
		softc->prd.md_primary = 0;
		softc->prd.md_flags = MP_DISPLAY;

		/*
		 * Enable video and clear interrupt
		 */
#ifdef sun2
		bwtwosetcr(softc->cr, BW2_VIDEOENABLEMASK, 1);
		bwtwosetcr(softc->cr, BW2_INTENABLEMASK, 0);
#else sun2
		if (softc->p4reg)
			*softc->p4reg = P4_REG_VIDEO | P4_REG_INTCLR;
		else {
			(void) setvideoenable(1);
			(void) setintrenable(0);
		}
#endif sun2
	}
	break;

#endif NWIN > 0

	case FBIOSVIDEO: {
		register int on = * (int *) data & FBVIDEO_ON;

#ifdef sun2
		bwtwosetcr(softc->cr, BW2_VIDEOENABLEMASK, on);
#else sun2
		if (softc->p4reg)
			*softc->p4reg = *softc->p4reg & 
				~(P4_REG_INTCLR | P4_REG_VIDEO) |
				(on ? P4_REG_VIDEO : 0);
		else
			setvideoenable((u_int) on);
#endif sun2
	}
	break;

	/* get video flags */
	case FBIOGVIDEO:
		* (int *) data = 
#ifdef sun2
			softc->cr->vc_video_en
#else sun2
			(softc->p4reg ?
				*softc->p4reg & P4_REG_VIDEO :
				enablereg & ENA_VIDEO)
#endif sun2
			? FBVIDEO_ON : FBVIDEO_OFF;
	break;

	default:
		return ENOTTY;

	} /* switch(cmd) */

	return 0;
}

bwtwointr()
{
	register struct bw2_softc *softc;
	int serviced = 0;

	for (softc = bw2_softc; softc < &bw2_softc[nbwtwo]; softc++) {
		if (!softc->image)
			continue;
#ifdef sun2
		if (softc->cr->vc_int) {
			bwtwosetcr(softc->cr, BW2_INTENABLEMASK, 0);
			serviced++;
		}
#else sun2
		if (softc->p4reg && *softc->p4reg & P4_REG_INTCLR) {
			*softc->p4reg |= P4_REG_INTCLR;
			serviced++;
		}
		(void) setintrenable(0);
#endif sun2
	}

	return serviced;
}


#ifdef sun2

/* Sun-2 support routines */

/*
 * Special access approach to video ctrl register needed because byte writes,
 * generated when do bit writes, replicates itself on the subsequent byte as
 * well (this is a hardware bug).  Thus, we need to access the register as a
 * word.  Also these routines assume that only one bit changes at a time.
 */
static
bwtwosetcr(bw2cr, mask, value)
	struct	bw2cr *bw2cr;
	short	mask;
	int	value;
{
	register short	w;

	/*
	 * Read word from video control register.
	 */
	w = *((short *)bw2cr);
	/*
	 * Modify bit as requested.
	 */
	if (value)
		w |= mask;
	else
		w &= ~mask;
	/*
	 * Write word back to video control register.
	 */
	*((short *)bw2cr) = w;
}

/*
 * Given the frame buffer virtual address, map in the control register.
 * Guess the location from the frame buffer's address space.
 */
static
bw2crmapin(bw2dev)
	register struct bw2dev *bw2dev;
{
	int page = fbgetpage((caddr_t) bw2dev);

	switch (page & PGT_MASK) {
	case BW2MB_PGT:
		page += btop(BW2MB_CR - BW2MB_FB);
		break;
	case BW2VME_PGT:
		page += btop(BW2VME_CR - BW2VME_FB);
		break;
	default:
		panic("bwtwoprobe: cannot find control register");
		/*NOTREACHED*/
	}

	fbmapin((caddr_t) &bw2dev->bw2cr, page, NBPG);
}
#endif sun2
#endif NBWTWO > 0
